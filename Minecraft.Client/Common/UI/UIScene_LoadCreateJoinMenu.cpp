#include "stdafx.h"

#include "UI.h"

#include "UIScene_LoadCreateJoinMenu.h"

// By Rockefort O Rockefeler

#include "..\..\..\Minecraft.World\StringHelpers.h"

#include "..\..\..\Minecraft.World\net.minecraft.world.item.h"

#include "..\..\..\Minecraft.World\net.minecraft.world.level.h"

#include "..\..\..\Minecraft.World\net.minecraft.world.level.chunk.storage.h"

#include "..\..\..\Minecraft.World\ConsoleSaveFile.h"

#include "..\..\..\Minecraft.World\ConsoleSaveFileOriginal.h"

#include "..\..\ProgressRenderer.h"

#include "..\..\MinecraftServer.h"

#include "..\..\TexturePackRepository.h"

#include "..\..\TexturePack.h"

#include "..\Network\SessionInfo.h"

#if defined(__PS3__) || defined(__ORBIS__) || defined(__PSVITA__)

#include "Common\Network\Sony\SonyHttp.h"

#include "Common\Network\Sony\SonyRemoteStorage.h"

#include "DLCTexturePack.h"

#endif

#if defined(__ORBIS__) || defined(__PSVITA__)

#include <ces.h>

#endif

#ifdef __PSVITA__

#include "message_dialog.h"

#endif



#ifdef _WINDOWS64

#include "..\..\..\Minecraft.World\NbtIo.h"

#include "..\..\..\Minecraft.World\compression.h"
#include "..\..\Windows64\KeyboardMouseInput.h"
#include "UISplitScreenHelpers.h"

#include <vector>



static bool IsGeneratedSaveTitle(const char *title)

{

    if(title == nullptr) return false;

    size_t len = strlen(title);

    if(len != 14) return false;

    for(size_t i = 0; i < len; ++i)

    {

        if(title[i] < '0' || title[i] > '9') return false;

    }

    return true;

}



static bool TryReadSaveThumbnailMetadata(const SAVE_INFO &saveInfo, unsigned int &uiHostOptions, DWORD &uiTexturePack)
{
    if(saveInfo.thumbnailData == nullptr || saveInfo.metaData.thumbnailSize == 0)
        return false;

    bool bHostOptionsRead = false;
    char szSeed[50] = {};
    uiHostOptions = 0;
    uiTexturePack = 0;
    app.GetImageTextData(saveInfo.thumbnailData, saveInfo.metaData.thumbnailSize, reinterpret_cast<unsigned char*>(&szSeed), uiHostOptions, bHostOptionsRead, uiTexturePack);
    return bHostOptionsRead;
}

static bool IsTutorialSaveMetadata(unsigned int uiHostOptions, DWORD uiTexturePack)
{
    if(uiTexturePack != TexturePackRepository::DEFAULT_TEXTURE_PACK_ID)
        return false;

    const unsigned int expectedTutorial = app.GetGameHostOption(uiHostOptions, eGameHostOption_Tutorial);
    return uiHostOptions == expectedTutorial;
}

#ifdef _WINDOWS64
static bool TryLoadThumbnailFromDisk_Win64(const char *saveFolder, PBYTE *outData, DWORD *outSize)
{
    if(outData) *outData = nullptr;
    if(outSize) *outSize = 0;
    if(saveFolder == nullptr || saveFolder[0] == '\0')
        return false;

    wchar_t wFolder[MAX_SAVEFILENAME_LENGTH];
    ZeroMemory(wFolder, sizeof(wFolder));
    int converted = MultiByteToWideChar(CP_UTF8, 0, saveFolder, -1, wFolder, MAX_SAVEFILENAME_LENGTH);
    if(converted <= 0)
    {
        mbstowcs(wFolder, saveFolder, MAX_SAVEFILENAME_LENGTH - 1);
    }

    wstring filePath = wstring(L"Windows64\\GameHDD\\") + wFolder + wstring(L"\\saveThumbnail.png");
    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if(hFile == INVALID_HANDLE_VALUE)
        return false;

    DWORD fileSize = GetFileSize(hFile, nullptr);
    if(fileSize == INVALID_FILE_SIZE || fileSize == 0)
    {
        CloseHandle(hFile);
        return false;
    }

    PBYTE buffer = new BYTE[fileSize];
    DWORD bytesRead = 0;
    BOOL ok = ReadFile(hFile, buffer, fileSize, &bytesRead, nullptr);
    CloseHandle(hFile);
    if(!ok || bytesRead == 0)
    {
        delete [] buffer;
        return false;
    }

    if(outData) *outData = buffer;
    if(outSize) *outSize = bytesRead;
    return true;
}
#endif
static wstring GetPreferredLevelGenSaveTitle(LevelGenerationOptions *levelGen)
{
    if(levelGen == nullptr)
        return L"";

    if(levelGen->isTutorial())
        return app.GetString(IDS_TUTORIALSAVENAME);

    LPCWSTR displayName = levelGen->getDisplayName();
    if(displayName != nullptr && displayName[0] != L'\0')
        return displayName;

    wstring defaultSaveName = levelGen->getDefaultSaveName();
    if(!defaultSaveName.empty())
        return defaultSaveName;

    LPCWSTR worldName = levelGen->getWorldName();
    if(worldName != nullptr && worldName[0] != L'\0')
        return worldName;

    return L"";
}

static wstring GetPackFallbackName(DWORD uiTexturePack)
{
    if(uiTexturePack == TexturePackRepository::DEFAULT_TEXTURE_PACK_ID)
        return L"";

    vector<LevelGenerationOptions *> *levelGenerators = app.getLevelGenerators();
    if(levelGenerators == nullptr)
        return L"";

    for(LevelGenerationOptions *levelGen : *levelGenerators)
    {
        if(levelGen == nullptr || levelGen->isTutorial())
            continue;

        if(levelGen->requiresTexturePack() && levelGen->getRequiredTexturePackId() == uiTexturePack)
        {
            wstring preferredTitle = GetPreferredLevelGenSaveTitle(levelGen);
            if(!preferredTitle.empty())
                return preferredTitle;
        }
    }

    return L"";
}

static wstring GetGeneratedSaveFallbackName(const SAVE_INFO &saveInfo)
{
    unsigned int uiHostOptions = 0;
    DWORD uiTexturePack = TexturePackRepository::DEFAULT_TEXTURE_PACK_ID;
    if(TryReadSaveThumbnailMetadata(saveInfo, uiHostOptions, uiTexturePack))
    {
        if(IsTutorialSaveMetadata(uiHostOptions, uiTexturePack))
            return app.GetString(IDS_TUTORIALSAVENAME);

        wstring packFallbackName = GetPackFallbackName(uiTexturePack);
        if(!packFallbackName.empty())
            return packFallbackName;
    }

    return app.GetString(IDS_CREATE_NEW_WORLD);
}
static bool TryLoadUiImageBytes(const wchar_t *fileName, std::vector<unsigned char> &outBytes)

{

    wchar_t modulePath[MAX_PATH] = {};

    GetModuleFileNameW(nullptr, modulePath, MAX_PATH);

    wchar_t *lastSlash = wcsrchr(modulePath, L'\\');

    if(lastSlash) *(lastSlash + 1) = L'\0';



    const wstring exeDir(modulePath);

    const wstring candidates[] =

    {

        exeDir + L"Common\\Media\\MediaWindows64\\Graphics\\" + fileName,

        exeDir + L"Graphics\\" + fileName,

        exeDir + fileName,

        exeDir + L"..\\..\\..\\..\\Minecraft.Client\\Common\\Media\\MediaWindows64\\Graphics\\" + fileName,
        exeDir + L"..\\..\\..\\..\\Minecraft.Client\\Windows64Media\\Graphics\\" + fileName,

        wstring(L"Common\\Media\\MediaWindows64\\Graphics\\") + fileName,

        wstring(L"Windows64Media\\Graphics\\") + fileName,

        wstring(L"Graphics\\") + fileName,

    };



    for (const wstring &fullPath : candidates)

    {

        HANDLE hFile = CreateFileW(fullPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

        if (hFile == INVALID_HANDLE_VALUE)

            continue;



        DWORD fileSize = GetFileSize(hFile, nullptr);

        if (fileSize == INVALID_FILE_SIZE || fileSize == 0)

        {

            CloseHandle(hFile);

            continue;

        }



        outBytes.resize(fileSize);

        DWORD bytesRead = 0;

        const BOOL ok = ReadFile(hFile, outBytes.data(), fileSize, &bytesRead, nullptr);

        CloseHandle(hFile);



        if (ok && bytesRead == fileSize)

            return true;

    }



    outBytes.clear();

    return false;

}



static void TrySetButtonListIcon(UIControl_SaveList &list, int itemIndex, const wchar_t *fileName, const wchar_t *textureName)

{

    std::vector<unsigned char> bytes;

    if(!TryLoadUiImageBytes(fileName, bytes) || bytes.empty())

        return;



    BYTE *copy = new BYTE[bytes.size()];

    memcpy(copy, bytes.data(), bytes.size());

    ui.registerSubstitutionTexture(textureName, copy, static_cast<DWORD>(bytes.size()));

    list.setTextureName(itemIndex, textureName);

}



static wstring ReadLevelNameFromSaveFile(const wstring& filePath)

{

    // Check for a worldname.txt sidecar written by the rename feature first

    size_t slashPos = filePath.rfind(L'\\');

    if (slashPos != wstring::npos)

    {

        wstring sidecarPath = filePath.substr(0, slashPos + 1) + L"worldname.txt";

        FILE *fr = nullptr;

        if (_wfopen_s(&fr, sidecarPath.c_str(), L"r") == 0 && fr)

        {

            char buf[128] = {};

            if (fgets(buf, sizeof(buf), fr))

            {

                int len = static_cast<int>(strlen(buf));

                while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r' || buf[len-1] == ' '))

                    buf[--len] = '\0';

                fclose(fr);

                if (len > 0)

                {

                    wchar_t wbuf[128] = {};

                    mbstowcs(wbuf, buf, 127);

                    return wstring(wbuf);

                }

            }

            else fclose(fr);

        }

    }



    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);

    if (hFile == INVALID_HANDLE_VALUE) return L"";



    DWORD fileSize = GetFileSize(hFile, nullptr);

    if (fileSize < 12 || fileSize == INVALID_FILE_SIZE) { CloseHandle(hFile); return L""; }



    unsigned char *rawData = new unsigned char[fileSize];

    DWORD bytesRead = 0;

    if (!ReadFile(hFile, rawData, fileSize, &bytesRead, nullptr) || bytesRead != fileSize)

    {

        CloseHandle(hFile);

        delete[] rawData;

        return L"";

    }

    CloseHandle(hFile);



    unsigned char *saveData = nullptr;

    unsigned int saveSize   = 0;

    bool freeSaveData = false;



    if (*(unsigned int*)rawData == 0)

    {

        // Compressed format: bytes 0-3=0, bytes 4-7=decompressed size, bytes 8+=compressed data

        unsigned int decompSize = *(unsigned int*)(rawData + 4);

        if (decompSize == 0 || decompSize > 128 * 1024 * 1024)

        {

            delete[] rawData;

            return L"";

        }

        saveData = new unsigned char[decompSize];

        Compression::getCompression()->Decompress(saveData, &decompSize, rawData + 8, fileSize - 8);

        saveSize     = decompSize;

        freeSaveData = true;

    }

    else

    {

        saveData = rawData;

        saveSize = fileSize;

    }



    wstring result = L"";

    if (saveSize >= 12)

    {

        unsigned int headerOffset = *(unsigned int*)saveData;

        unsigned int numEntries   = *(unsigned int*)(saveData + 4);

        const unsigned int entrySize = sizeof(FileEntrySaveData);



        if (headerOffset < saveSize && numEntries > 0 && numEntries < 10000 &&

            headerOffset + numEntries * entrySize <= saveSize)

        {

            FileEntrySaveData *table = (FileEntrySaveData *)(saveData + headerOffset);

            for (unsigned int i = 0; i < numEntries; i++)

            {

                if (wcscmp(table[i].filename, L"level.dat") == 0)

                {

                    unsigned int off = table[i].startOffset;

                    unsigned int len = table[i].length;

                    if (off >= 12 && off + len <= saveSize && len > 0 && len < 4 * 1024 * 1024)

                    {

                        byteArray ba;

                        ba.data   = (byte*)(saveData + off);

                        ba.length = len;

                        CompoundTag *root = NbtIo::decompress(ba);

                        if (root != nullptr)

                        {

                            CompoundTag *dataTag = root->getCompound(L"Data");

                            if (dataTag != nullptr)

                                result = dataTag->getString(L"LevelName");

                            delete root;

                        }

                    }

                    break;

                }

            }

        }

    }



    if (freeSaveData) delete[] saveData;

    delete[] rawData;

    // "world" is the engine default - it means no real name was ever set,

    // so return empty to let the caller fall back to the save filename (timestamp).

    if (result == L"world") result = L"";

    return result;

}

#endif





#ifdef SONY_REMOTE_STORAGE_DOWNLOAD

unsigned long UIScene_LoadCreateJoinMenu::m_ulFileSize=0L;

wstring UIScene_LoadCreateJoinMenu::m_wstrStageText=L"";

bool UIScene_LoadCreateJoinMenu::m_bSaveTransferRunning = false;

#endif





#define JOIN_LOAD_ONLINE_TIMER_ID 0

#define JOIN_LOAD_ONLINE_TIMER_TIME 100



#ifdef _XBOX

#define CHECKFORAVAILABLETEXTUREPACKS_TIMER_ID 3

#define CHECKFORAVAILABLETEXTUREPACKS_TIMER_TIME 50

#endif



#ifdef _XBOX_ONE

UIScene_LoadCreateJoinMenu::ESaveTransferFiles UIScene_LoadCreateJoinMenu::s_eSaveTransferFile;

unsigned long UIScene_LoadCreateJoinMenu::s_ulFileSize=0L;

byteArray UIScene_LoadCreateJoinMenu::s_transferData = byteArray();

wstring UIScene_LoadCreateJoinMenu::m_wstrStageText=L"";



#ifdef _DEBUG_MENUS_ENABLED

C4JStorage::SAVETRANSFER_FILE_DETAILS UIScene_LoadCreateJoinMenu::m_debugTransferDetails;

#endif

#endif



int UIScene_LoadCreateJoinMenu::LoadSaveDataThumbnailReturned(LPVOID lpParam,PBYTE pbThumbnail,DWORD dwThumbnailBytes)

{

    UIScene_LoadCreateJoinMenu *pClass= static_cast<UIScene_LoadCreateJoinMenu *>(lpParam);



    app.DebugPrintf("Received data for save thumbnail\n");

#ifdef _WINDOWS64
    if((pbThumbnail == nullptr || dwThumbnailBytes == 0) &&
       pClass != nullptr &&
       pClass->m_iRequestingThumbnailId >= 0 &&
       pClass->m_iRequestingThumbnailId < pClass->m_iSaveDetailsCount)
    {
        PBYTE pbCachedThumbnail = nullptr;
        DWORD dwCachedThumbnailBytes = 0;
        StorageManager.GetSaveCacheFileInfo(pClass->m_saveDetails[pClass->m_iRequestingThumbnailId].saveId, &pbCachedThumbnail, &dwCachedThumbnailBytes);
        if(pbCachedThumbnail && dwCachedThumbnailBytes)
        {
            app.DebugPrintf("LoadCreateJoin thumbnail fallback hit cache size %d\n", dwCachedThumbnailBytes);
            pbThumbnail = pbCachedThumbnail;
            dwThumbnailBytes = dwCachedThumbnailBytes;
        }
    }
#endif

    if(pbThumbnail && dwThumbnailBytes)

    {

        pClass->m_saveDetails[pClass->m_iRequestingThumbnailId].pbThumbnailData = new BYTE[dwThumbnailBytes];

        memcpy(pClass->m_saveDetails[pClass->m_iRequestingThumbnailId].pbThumbnailData, pbThumbnail, dwThumbnailBytes);

        pClass->m_saveDetails[pClass->m_iRequestingThumbnailId].dwThumbnailSize = dwThumbnailBytes;

    }

    else

    {

        pClass->m_saveDetails[pClass->m_iRequestingThumbnailId].pbThumbnailData = nullptr;

        pClass->m_saveDetails[pClass->m_iRequestingThumbnailId].dwThumbnailSize = 0;

        app.DebugPrintf("Save thumbnail data is nullptr, or has size 0\n");

    }

    pClass->m_bSaveThumbnailReady = true;



    return 0;

}



int UIScene_LoadCreateJoinMenu::LoadSaveCallback(LPVOID lpParam,bool bRes)

{

    //UIScene_LoadCreateJoinMenu *pClass= (UIScene_LoadCreateJoinMenu *)lpParam;

    // Get the save data now

    if(bRes)

    {

        app.DebugPrintf("Loaded save OK\n");

    }

    return 0;

}



UIScene_LoadCreateJoinMenu::UIScene_LoadCreateJoinMenu(int iPad, void *initData, UILayer *parentLayer) : UIScene(iPad, parentLayer)

{

    constexpr uint64_t MAXIMUM_SAVE_STORAGE = 4LL * 1024LL * 1024LL * 1024LL;



    // Setup all the Iggy references we need for this scene

    initialiseMovie();

    app.SetLiveLinkRequired( true );



    m_iRequestingThumbnailId = 0;

    m_iSaveInfoC=0;

    m_bIgnoreInput = false;

    m_bShowingPartyGamesOnly = false;

    m_bInParty = false;

    m_activeTab = eTab_Load;
    m_bPendingSaveSizeBarRefresh = false;
    m_bPendingJoinTabAvailabilityRefresh = false;
    m_bPendingJoinVisualRefresh = false;
    m_bRebuildingJoinVisual = false;
#ifdef _WINDOWS64
    m_lastHoverMouseX = -1;
    m_lastHoverMouseY = -1;
    m_mouseHoverTracked = false;
    m_hoverBaseIndexLoad = 0;
    m_hoverBaseIndexCreate = 0;
    m_hoverBaseIndexJoin = 0;
#endif

    m_currentSessions = nullptr;

    m_iState=e_SavesIdle;

    //m_bRetrievingSaveInfo=false;



    m_buttonListSaves.init(eControl_SavesList);

    m_buttonListNewGames.init(eControl_NewGamesList);

    m_buttonListGames.init(eControl_GamesList);



    m_labelSavesListTitle.init( L"Load" );

    m_labelCreateListTitle.init( IDS_TOOLTIPS_CREATE );

    m_labelJoinListTitle.init( L"Join" );

        m_bHasNoGamesLabel = false;

    m_controlSavesTimer.setVisible( true );

    m_controlNewGameTimer.setVisible( true );

    m_controlJoinTimer.setVisible( false );





#if defined(_XBOX_ONE) || defined(__ORBIS__) || defined(_WINDOWS64)

    m_spaceIndicatorSaves.init(L"",eControl_SpaceIndicator,0, MAXIMUM_SAVE_STORAGE);

#endif

    m_bUpdateSaveSize = false;



    m_bAllLoaded = false;

    m_bRetrievingSaveThumbnails = false;

    m_bSaveThumbnailReady = false;

    m_bExitScene=false;

    m_pSaveDetails=nullptr;

    m_bSavesDisplayed=false;

    m_saveDetails = nullptr;

    m_iSaveDetailsCount = 0;

    m_iTexturePacksNotInstalled = 0;

	m_bCopying = false;

	m_bCopyingCancelled = false;



#ifndef _XBOX_ONE

    m_bSaveTransferCancelled=false;

    m_bSaveTransferInProgress=false;

#endif

	m_eAction = eAction_None;



    m_bMultiplayerAllowed = ProfileManager.IsSignedInLive( m_iPad ) && ProfileManager.AllowedToPlayMultiplayer(m_iPad);



#ifdef _XBOX_ONE

	// 4J-PB - in order to buy the skin packs & texture packs, we need the signed offer ids for them, which we get in the availability info

	// we need to retrieve this info though, so do it here

	app.AddDLCRequest(e_Marketplace_Content); // content is skin packs, texture packs and mash-up packs

#endif





    int iLB = -1;



#ifdef _XBOX

    XPARTY_USER_LIST partyList;



    if((XPartyGetUserList(  &partyList ) != XPARTY_E_NOT_IN_PARTY ) && (partyList.dwUserCount>1))

    {

        m_bInParty=true;

    }

    else

    {

        m_bInParty=false;

    }

#endif



#if defined(__PS3__) || defined(__ORBIS__) || defined(__PSVITA__) || defined(_DURANGO) || defined(_WINDOWS64)

    // Always clear the saves when we enter this menu

    StorageManager.ClearSavesInfo();

#endif



    // block input if we're waiting for DLC to install, and wipe the saves list. The end of dlc mounting custom message will fill the list again

    if(app.StartInstallDLCProcess(m_iPad)==true || app.DLCInstallPending())

    {

        // if we're waiting for DLC to mount, don't fill the save list. The custom message on end of dlc mounting will do that

        m_bIgnoreInput = true;

    }

    else

    {

        Initialise();

    }



#ifdef __PSVITA__

    if(CGameNetworkManager::usingAdhocMode() && SQRNetworkManager_AdHoc_Vita::GetAdhocStatus())

    {

        g_NetworkManager.startAdhocMatching();			// create the client matching context and clear out the friends list

    }



#endif



    UpdateGamesList();



    g_NetworkManager.SetSessionsUpdatedCallback( &UpdateGamesListCallback, this );



    m_initData= new JoinMenuInitData();



    // 4J Stu - Fix for #12530 -TCR 001 BAS Game Stability: Title will crash if the player disconnects while starting a new world and then opts to play the tutorial once they have been returned to the Main Menu.

    MinecraftServer::resetFlags();



    // If we're not ignoring input, then we aren't still waiting for the DLC to mount, and can now check for corrupt dlc. Otherwise this will happen when the dlc has finished mounting.

    if( !m_bIgnoreInput)

    {

        app.m_dlcManager.checkForCorruptDLCAndAlert();

    }



    // 4J-PB - Only Xbox will not have trial DLC patched into the game

#ifdef _XBOX

    // 4J-PB - there may be texture packs we don't have, so use the info from TMS for this



    DLC_INFO *pDLCInfo=nullptr;



    // first pass - look to see if there are any that are not in the list

    bool bTexturePackAlreadyListed;

    bool bNeedToGetTPD=false;

    Minecraft *pMinecraft = Minecraft::GetInstance();

    int texturePacksCount = pMinecraft->skins->getTexturePackCount();



    for(unsigned int i = 0; i < app.GetDLCInfoTexturesOffersCount(); ++i)

    {

        bTexturePackAlreadyListed=false;

#if defined(__PS3__) || defined(__ORBIS__)

        char *pchDLCName=app.GetDLCInfoTextures(i);

        pDLCInfo=app.GetDLCInfo(pchDLCName);

#else

        ULONGLONG ull=app.GetDLCInfoTexturesFullOffer(i);

        pDLCInfo=app.GetDLCInfoForFullOfferID(ull);

#endif

        for(unsigned int i = 0; i < texturePacksCount; ++i)

        {

            TexturePack *tp = pMinecraft->skins->getTexturePackByIndex(i);

            if(pDLCInfo && pDLCInfo->iConfig==tp->getDLCParentPackId())

            {

                bTexturePackAlreadyListed=true;

            }

        }

        if(bTexturePackAlreadyListed==false)

        {

            // some missing

            bNeedToGetTPD=true;



            m_iTexturePacksNotInstalled++;

        }

    }



    if(bNeedToGetTPD==true)

    {

        // add a TMS request for them

        app.DebugPrintf("+++ Adding TMSPP request for texture pack data\n");

        app.AddTMSPPFileTypeRequest(e_DLC_TexturePackData);

        m_iConfigA= new int [m_iTexturePacksNotInstalled];

        m_iTexturePacksNotInstalled=0;



        for(unsigned int i = 0; i < app.GetDLCInfoTexturesOffersCount(); ++i)

        {

            bTexturePackAlreadyListed=false;

#if defined(__PS3__) || defined(__ORBIS__)

            char *pchDLCName=app.GetDLCInfoTextures(i);

            pDLCInfo=app.GetDLCInfo(pchDLCName);

#else

            ULONGLONG ull=app.GetDLCInfoTexturesFullOffer(i);

            pDLCInfo=app.GetDLCInfoForFullOfferID(ull);

#endif

            for(unsigned int i = 0; i < texturePacksCount; ++i)

            {

                TexturePack *tp = pMinecraft->skins->getTexturePackByIndex(i);

                if(pDLCInfo->iConfig==tp->getDLCParentPackId())

                {

                    bTexturePackAlreadyListed=true;

                }

            }

            if(bTexturePackAlreadyListed==false)

            {

                m_iConfigA[m_iTexturePacksNotInstalled++]=pDLCInfo->iConfig;

            }

        }

    }



    addTimer(CHECKFORAVAILABLETEXTUREPACKS_TIMER_ID,CHECKFORAVAILABLETEXTUREPACKS_TIMER_TIME);

#endif



#ifdef SONY_REMOTE_STORAGE_DOWNLOAD

    m_eSaveTransferState = eSaveTransfer_Idle;

#endif

}





UIScene_LoadCreateJoinMenu::~UIScene_LoadCreateJoinMenu()

{

    g_NetworkManager.SetSessionsUpdatedCallback( nullptr, nullptr );

    app.SetLiveLinkRequired( false );



    if (m_currentSessions)

    {

        for (const auto& it : *m_currentSessions)

            delete it;

        delete m_currentSessions;

        m_currentSessions = nullptr;

    }



#if TO_BE_IMPLEMENTED

    // Reset the background downloading, in case we changed it by attempting to download a texture pack

    XBackgroundDownloadSetMode(XBACKGROUND_DOWNLOAD_MODE_AUTO);

#endif



    if(m_saveDetails)

    {

        for(int i = 0; i < m_iSaveDetailsCount; ++i)

        {

            delete m_saveDetails[i].pbThumbnailData;

        }

        delete [] m_saveDetails;

    }

}



void UIScene_LoadCreateJoinMenu::updateTooltips()
{
#if defined __PS3__ || defined __ORBIS__ || defined __PSVITA__
    if(m_eSaveTransferState!=eSaveTransfer_Idle)
    {
        return;
    }
#endif

    int iRB = -1;
    int iY = -1;
    int iLB = -1;
    int iX = -1;
    int iLS = IDS_TOOLTIPS_NAVIGATE;

    if ((m_activeTab == eTab_Load) && (m_pSaveDetails != nullptr) && (m_pSaveDetails->iSaveC > 0) && (m_buttonListSaves.getItemCount() > 0))
    {
        if(StorageManager.GetSaveDisabled())
        {
            iY = IDS_TOOLTIPS_DELETESAVE;
        }
        else if(StorageManager.EnoughSpaceForAMinSaveGame())
        {
            iY = IDS_TOOLTIPS_SAVEOPTIONS;
        }
        else
        {
            iY = IDS_TOOLTIPS_DELETESAVE;
        }
    }
    else if(DoesMashUpWorldHaveFocus())
    {
        iY = IDS_TOOLTIPS_HIDE;
    }


#if defined(__PS3__) || defined(__ORBIS__) || defined(__PSVITA__)
    if(m_iPad == ProfileManager.GetPrimaryPad())
        iY = IDS_TOOLTIPS_GAME_INVITES;
#endif

    if(ProfileManager.IsFullVersion()==false)
    {
        iRB = -1;
    }
    else if(StorageManager.GetSaveDisabled())
    {
#ifdef _XBOX
        iX = IDS_TOOLTIPS_SELECTDEVICE;
#endif
    }
    else
    {
#if defined _XBOX_ONE
        if(ProfileManager.IsSignedInLive(m_iPad))
        {
            iX = IDS_TOOLTIPS_SAVETRANSFER_DOWNLOAD;
        }
#elif defined SONY_REMOTE_STORAGE_DOWNLOAD
        bool bSignedInLive = ProfileManager.IsSignedInLive(m_iPad);
        if(bSignedInLive)
        {
            iX = IDS_TOOLTIPS_SAVETRANSFER_DOWNLOAD;
        }
#else
        iX = IDS_TOOLTIPS_CHANGEDEVICE;
#endif
    }

    ui.SetTooltips(DEFAULT_XUI_MENU_USER, IDS_TOOLTIPS_SELECT, IDS_TOOLTIPS_BACK, iX, iY, -1, -1, iLB, iRB, iLS, -1, -1, true);
}

//
void UIScene_LoadCreateJoinMenu::Initialise()

{

    m_iSaveListIndex = 0;

	m_iGameListIndex = 0;

#ifdef _WINDOWS64

	m_addServerPhase = eAddServer_Idle;

#endif



    m_iDefaultButtonsC = 0;

	m_iMashUpButtonsC=0;



    // Check if we're in the trial version

    if(ProfileManager.IsFullVersion()==false)

    {





        AddDefaultButtons();



#if TO_BE_IMPLEMENTED

        m_pSavesList->SetCurSelVisible(0);

#endif

    }

    else if(StorageManager.GetSaveDisabled())

    {

#if defined(__PS3__) || defined(__ORBIS__) || defined (__PSVITA__)

        GetSaveInfo();
        m_controlSavesTimer.setVisible(true);

#else



#if TO_BE_IMPLEMENTED

        if(StorageManager.GetSaveDeviceSelected(m_iPad))

#endif

        {

            // saving is disabled, but we should still be able to load from a selected save device







            GetSaveInfo();
            m_controlSavesTimer.setVisible(true);

        }

#if TO_BE_IMPLEMENTED

        else

        {

            AddDefaultButtons();

            m_controlSavesTimer.setVisible( false );

        }

#endif

#endif // __PS3__ || __ORBIS

    }

    else

    {

        // 4J-PB - we need to check that there is enough space left to create a copy of the save (for a rename)

        bool bCanRename = StorageManager.EnoughSpaceForAMinSaveGame();



        GetSaveInfo();
        m_controlSavesTimer.setVisible(true);

    }



    m_bIgnoreInput=true;

    app.m_dlcManager.checkForCorruptDLCAndAlert();

}



void UIScene_LoadCreateJoinMenu::updateComponents()

{

    m_parentLayer->showComponent(m_iPad,eUIComponent_Panorama,true);

    m_parentLayer->showComponent(m_iPad,eUIComponent_Logo,false);

}



void UIScene_LoadCreateJoinMenu::handleDestroy()

{

#ifdef __PSVITA__

	app.DebugPrintf("missing InputManager.DestroyKeyboard on Vita !!!!!!\n");

#endif



	// shut down the keyboard if it is displayed

#if ( defined __PS3__ || defined __ORBIS__ || defined _DURANGO)

	InputManager.DestroyKeyboard();

#endif

}





void UIScene_LoadCreateJoinMenu::handleGainFocus(bool navBack)

{

    UIScene::handleGainFocus(navBack);
    updateTooltips();
    SyncMovieTab();
    m_bPendingSaveSizeBarRefresh = true;
    m_bPendingJoinTabAvailabilityRefresh = true;
    addTimer(JOIN_LOAD_ONLINE_TIMER_ID,JOIN_LOAD_ONLINE_TIMER_TIME);

    if(navBack)

    {

        app.SetLiveLinkRequired( true );

        m_bMultiplayerAllowed = ProfileManager.IsSignedInLive( m_iPad ) && ProfileManager.AllowedToPlayMultiplayer(m_iPad);

        m_bIgnoreInput=false;

        if(app.StartInstallDLCProcess(m_iPad)==false)

        {

            m_bIgnoreInput=false;

        }

        else

        {

            m_bIgnoreInput=true;

            m_buttonListSaves.clearList();

            m_controlSavesTimer.setVisible(true);

        }



        if( m_bMultiplayerAllowed )

        {

#if TO_BE_IMPLEMENTED

            HXUICLASS hClassFullscreenProgress = XuiFindClass( L"CScene_FullscreenProgress" );

            HXUICLASS hClassConnectingProgress = XuiFindClass( L"CScene_ConnectingProgress" );

            if( XuiIsInstanceOf( hSceneFrom, hClassFullscreenProgress ) || XuiIsInstanceOf( hSceneFrom, hClassConnectingProgress ) )

            {

                UpdateGamesList();

            }

#endif

        }

        else

        {

            m_buttonListGames.clearList();

            m_controlJoinTimer.setVisible(false);

            if(m_bHasNoGamesLabel) m_labelNoGames.setVisible(false);

#if TO_BE_IMPLEMENTED

            m_SavesList.InitFocus(m_iPad);

#endif

        }



        if(app.GetCorruptSaveDeleted())

        {

            m_iState=e_SavesRepopulateAfterDelete;

            app.SetCorruptSaveDeleted(false);

        }

    }

}

void UIScene_LoadCreateJoinMenu::handleLoseFocus()

{

    // Kill load online timer

    killTimer(JOIN_LOAD_ONLINE_TIMER_ID);

}



UIControl* UIScene_LoadCreateJoinMenu::GetMainPanel()

{
    // This movie does not reliably expose a root "MainPanel" value path in all
    // forks/builds. Mouse hover hit-testing in UIController depends on the main
    // panel for offsets and descendant filtering, so return the currently-active
    // tab container instead of the phantom root panel.
    switch (m_activeTab)
    {
    case eTab_Load:
        return &m_controlLoadGame;
    case eTab_Create:
        return &m_controlNewGame;
    case eTab_Join:
        return &m_controlJoinGame;
    default:
        return nullptr;
    }

}

#ifdef _WINDOWS64
bool UIScene_LoadCreateJoinMenu::ConvertMouseToSceneCoords(float &sceneMouseX, float &sceneMouseY)
{
    if (g_KBMInput.IsMouseGrabbed() || !g_KBMInput.IsKBMActive())
        return false;

    sceneMouseX = static_cast<float>(g_KBMInput.GetMouseX());
    sceneMouseY = static_cast<float>(g_KBMInput.GetMouseY());

    extern HWND g_hWnd;
    RECT rc = {};
    if (!g_hWnd || !GetClientRect(g_hWnd, &rc))
        return false;

    const int winW = rc.right - rc.left;
    const int winH = rc.bottom - rc.top;
    if (winW <= 0 || winH <= 0)
        return false;

    const float screenX = sceneMouseX * (ui.getScreenWidth() / static_cast<float>(winW));
    const float screenY = sceneMouseY * (ui.getScreenHeight() / static_cast<float>(winH));

    C4JRender::eViewportType vp = GetParentLayer()->getViewport();
    S32 displayW = 0, displayH = 0;

    F32 vpOriginX = 0, vpOriginY = 0, vpW = 0, vpH = 0;
    GetViewportRect(ui.getScreenWidth(), ui.getScreenHeight(), vp, vpOriginX, vpOriginY, vpW, vpH);

    S32 fitW = 0, fitH = 0, fitOffsetX = 0, fitOffsetY = 0;
    Fit16x9(vpW, vpH, fitW, fitH, fitOffsetX, fitOffsetY);

    const S32 originX = static_cast<S32>(vpOriginX) + fitOffsetX;
    const S32 originY = static_cast<S32>(vpOriginY) + fitOffsetY;
    displayW = fitW;
    displayH = fitH;

    if (displayW <= 0 || displayH <= 0)
        return false;

    sceneMouseX = (screenX - originX) * (static_cast<float>(getRenderWidth()) / static_cast<float>(displayW));
    sceneMouseY = (screenY - originY) * (static_cast<float>(getRenderHeight()) / static_cast<float>(displayH));
    return true;
}

void UIScene_LoadCreateJoinMenu::GetAbsoluteControlRect(UIControl *pControl, S32 &x, S32 &y, S32 &w, S32 &h)
{
    x = y = w = h = 0;
    if (!pControl)
        return;

    pControl->UpdateControl();
    x = pControl->getXPos();
    y = pControl->getYPos();
    w = pControl->getWidth();
    h = pControl->getHeight();

    UIControl *panel = pControl->getParentPanel();
    while (panel)
    {
        panel->UpdateControl();
        x += panel->getXPos();
        y += panel->getYPos();
        panel = panel->getParentPanel();
    }
}

void UIScene_LoadCreateJoinMenu::UpdateMouseHoverForActiveTab()
{
    if (!hasFocus(m_iPad) || getMovie() == nullptr || g_KBMInput.IsMouseGrabbed() || !g_KBMInput.IsKBMActive())
        return;

    if (m_bIgnoreInput)
        return;

    const int rawMouseX = g_KBMInput.GetMouseX();
    const int rawMouseY = g_KBMInput.GetMouseY();
    if (m_mouseHoverTracked && rawMouseX == m_lastHoverMouseX && rawMouseY == m_lastHoverMouseY)
        return;

    m_lastHoverMouseX = rawMouseX;
    m_lastHoverMouseY = rawMouseY;
    m_mouseHoverTracked = true;

    float sceneMouseX = 0.0f;
    float sceneMouseY = 0.0f;
    if (!ConvertMouseToSceneCoords(sceneMouseX, sceneMouseY))
        return;

    UIControl_ButtonList *pActiveList = nullptr;
    switch (m_activeTab)
    {
    case eTab_Load:
        pActiveList = &m_buttonListSaves;
        break;
    case eTab_Create:
        pActiveList = &m_buttonListNewGames;
        break;
    case eTab_Join:
        pActiveList = &m_buttonListGames;
        break;
    default:
        break;
    }

    if (pActiveList == nullptr || pActiveList->getHidden() || !pActiveList->getVisible())
        return;

    S32 cx = 0, cy = 0, cw = 0, ch = 0;
    GetAbsoluteControlRect(pActiveList, cx, cy, cw, ch);
    if (cw <= 0 || ch <= 0)
        return;

    if (sceneMouseX < cx || sceneMouseX > cx + cw || sceneMouseY < cy || sceneMouseY > cy + ch)
        return;

    const int itemCount = pActiveList->getItemCount();
    if (itemCount <= 0)
        return;

    int visibleRows = 7;
    float topInset = 6.0f;
    float bottomInset = 22.0f;
    switch (m_activeTab)
    {
    case eTab_Load:
        visibleRows = 7;
        bottomInset = 22.0f;
        break;
    case eTab_Create:
        visibleRows = 7;
        bottomInset = 22.0f;
        break;
    case eTab_Join:
        visibleRows = 7;
        bottomInset = 22.0f;
        break;
    default:
        break;
    }
    const float contentTop = static_cast<float>(cy) + topInset;
    const float contentBottom = static_cast<float>(cy + ch) - bottomInset;
    if (contentBottom <= contentTop)
        return;

    if (sceneMouseY < contentTop || sceneMouseY > contentBottom)
        return;

    const float rowHeight = (contentBottom - contentTop) / static_cast<float>(visibleRows);
    if (rowHeight <= 0.0f)
        return;

    const float hoverOffset = (sceneMouseY - contentTop) / rowHeight;
    int hoverRow = static_cast<int>(hoverOffset);
    if (hoverRow < 0)
        hoverRow = 0;
    if (hoverRow >= visibleRows)
        hoverRow = visibleRows - 1;
    const float hoverRowFraction = hoverOffset - static_cast<float>(hoverRow);

    int *pBaseIndex = nullptr;
    switch (m_activeTab)
    {
    case eTab_Load:
        pBaseIndex = &m_hoverBaseIndexLoad;
        break;
    case eTab_Create:
        pBaseIndex = &m_hoverBaseIndexCreate;
        break;
    case eTab_Join:
        pBaseIndex = &m_hoverBaseIndexJoin;
        break;
    default:
        return;
    }

    const int maxBaseIndex = (itemCount - visibleRows > 0) ? (itemCount - visibleRows) : 0;
    if (*pBaseIndex < 0)
        *pBaseIndex = 0;
    if (*pBaseIndex > maxBaseIndex)
        *pBaseIndex = maxBaseIndex;

    if (itemCount > visibleRows)
    {
        if (hoverRow == 0 && hoverRowFraction < 0.30f && *pBaseIndex > 0)
        {
            --(*pBaseIndex);
        }
        else if (hoverRow == (visibleRows - 1) && hoverRowFraction > 0.70f && *pBaseIndex < maxBaseIndex)
        {
            ++(*pBaseIndex);
        }
    }

    const int targetChildIndex = ((*pBaseIndex + hoverRow) < (itemCount - 1)) ? (*pBaseIndex + hoverRow) : (itemCount - 1);

    switch (m_activeTab)
    {
    case eTab_Load:
        if (m_iSaveListIndex == targetChildIndex)
            return;
        m_iSaveListIndex = targetChildIndex;
        m_bUpdateSaveSize = true;
        break;

    case eTab_Create:
        if (m_iNewGameListIndex == targetChildIndex)
            return;
        m_iNewGameListIndex = targetChildIndex;
        break;

    case eTab_Join:
    {
#ifdef _WINDOWS64
        const int targetGameIndex = targetChildIndex - 1;
#else
        const int targetGameIndex = targetChildIndex;
#endif
        if (m_iGameListIndex == targetGameIndex)
            return;
        m_iGameListIndex = targetGameIndex;
        break;
    }

    default:
        return;
    }

    updateTooltips();
    m_bPendingSaveSizeBarRefresh = true;
    m_bPendingJoinTabAvailabilityRefresh = true;
}
#endif
wstring UIScene_LoadCreateJoinMenu::getMoviePath()

{
    return L"LoadCreateJoinMenu";

}



int UIScene_LoadCreateJoinMenu::GetMovieTabFromFocus()

{

    if (DoesGamesListHaveFocus())

        return 2;



    if (m_buttonListNewGames.hasFocus())

        return 1;



    if (m_buttonListSaves.hasFocus())

        return 0;



    return 0;

}

void UIScene_LoadCreateJoinMenu::SetMovieTab(int tab)

{

    if (getMovie() == nullptr)

        return;



    IggyDataValue result;

    IggyDataValue value[1];

    value[0].type = IGGY_DATATYPE_number;

    value[0].number = static_cast<F64>(tab);

    IggyPlayerCallMethodRS(getMovie(), &result, IggyPlayerRootPath(getMovie()), m_funcSetActiveTab, 1, value);

}



void UIScene_LoadCreateJoinMenu::SyncMovieTab()

{

    SetMovieTab(GetMovieTabFromFocus());

}



void UIScene_LoadCreateJoinMenu::SetActiveTab(ELoadCreateJoinTab tab, bool setFocus)

{
    const bool enteringJoin = (m_activeTab != eTab_Join) && (tab == eTab_Join);
    m_activeTab = tab;

    if (m_activeTab == eTab_Join && !m_bRebuildingJoinVisual && (enteringJoin || m_bPendingJoinVisualRefresh))
    {
        RebuildJoinGamesListVisual(true);
        m_bPendingJoinVisualRefresh = false;
    }

    SetMovieTab((int)tab);

    ApplyTabVisibility(setFocus);

}



void UIScene_LoadCreateJoinMenu::ApplyTabVisibility(bool setFocus)

{
    // keep load create and join logic split here or focus gets weird

    const bool showLoad = (m_activeTab == eTab_Load);

    const bool showCreate = (m_activeTab == eTab_Create);

    const bool showJoin = (m_activeTab == eTab_Join);



    m_controlLoadGame.setVisible(showLoad);

    m_controlLoadGamePanel.setVisible(showLoad);

    m_buttonListSaves.setVisible(showLoad);



    m_controlNewGame.setVisible(showCreate);

    m_controlNewGamePanel.setVisible(showCreate);

    m_buttonListNewGames.setVisible(showCreate);



    m_controlJoinGame.setVisible(showJoin);

    m_controlJoinGamePanel.setVisible(showJoin);

    m_buttonListGames.setVisible(showJoin);

    if (m_bHasNoGamesLabel)
        m_labelNoGames.setVisible(showJoin && m_buttonListGames.getItemCount() == 0);



    if (setFocus)

    {

        if (showLoad)

            SetFocusToElement(eControl_SavesList);

        else if (showCreate)

            SetFocusToElement(eControl_NewGamesList);

        else if (showJoin)

            SetFocusToElement(eControl_GamesList);

    }


    updateTooltips();

}

void UIScene_LoadCreateJoinMenu::UpdateJoinTabAvailability()

{

    if (getMovie() == nullptr)

        return;


    IggyDataValue result;

    IggyDataValue value[1];

    value[0].type = IGGY_DATATYPE_boolean;

    value[0].boolval = false;

    IggyPlayerCallMethodRS(getMovie(), &result, IggyPlayerRootPath(getMovie()), m_funcSetMatchesAvailable, 1, value);

}



void UIScene_LoadCreateJoinMenu::UpdateSaveSizeBarVisibility()

{

    if (getMovie() == nullptr)

        return;



    const bool showSaveSizeBar = (m_activeTab == eTab_Load);

    IggyDataValue result;

    IggyDataValue value[1];

    value[0].type = IGGY_DATATYPE_boolean;

    value[0].boolval = showSaveSizeBar;

    IggyPlayerCallMethodRS(getMovie(), &result, IggyPlayerRootPath(getMovie()), m_funcShowSaveSizeBar, 1, value);

}

void UIScene_LoadCreateJoinMenu::tick()

{

    UIScene::tick();

#if (defined  __PS3__  || defined __ORBIS__ || defined _DURANGO || defined _WINDOWS64 || defined __PSVITA__)

    if(m_bExitScene) // navigate forward or back

    {

        if(!m_bRetrievingSaveThumbnails)

        {

            // need to wait for any callback retrieving thumbnail to complete

            navigateBack();

        }

    }

    // Stop loading thumbnails if we navigate forwards

    if(hasFocus(m_iPad))

    {

#ifdef SONY_REMOTE_STORAGE_DOWNLOAD

		// if the loadOrJoin menu has focus again, we can clear the saveTransfer flag now. Added so we can delay the ehternet disconnect till it's cleaned up

		if(m_eSaveTransferState == eSaveTransfer_Idle)

			m_bSaveTransferRunning = false;

#endif

#if defined(_XBOX_ONE) || defined(__ORBIS__) || defined(_WINDOWS64)

        if(m_bUpdateSaveSize)

        {

            if((m_activeTab == eTab_Load) && (m_pSaveDetails != nullptr) && (m_pSaveDetails->iSaveC > 0) && DoesSavesListHaveFocus() && (m_buttonListSaves.getItemCount() > 0) && (m_iSaveListIndex >= 0) && (m_iSaveListIndex < m_buttonListSaves.getItemCount()))

            {

                m_spaceIndicatorSaves.selectSave(m_iSaveListIndex);

            }

            else

            {

                m_spaceIndicatorSaves.selectSave(-1);

            }

            UpdateSaveSizeBarVisibility();

            m_bUpdateSaveSize = false;

        }

        if(m_bPendingSaveSizeBarRefresh)
        {
            UpdateSaveSizeBarVisibility();
            m_bPendingSaveSizeBarRefresh = false;
        }

        if(m_bPendingJoinTabAvailabilityRefresh)
        {
            UpdateJoinTabAvailability();
            m_bPendingJoinTabAvailabilityRefresh = false;
        }

#endif

        // Display the saves if we have them

        if(!m_bSavesDisplayed)

        {

            m_pSaveDetails=StorageManager.ReturnSavesInfo();

            if(m_pSaveDetails!=nullptr)

            {

                //CD - Fix - Adding define for ORBIS/XBOXONE

#if defined(_XBOX_ONE) || defined(__ORBIS__) || defined(_WINDOWS64)

                m_spaceIndicatorSaves.reset();

#endif



                AddDefaultButtons();

                m_bSavesDisplayed=true;

                UpdateGamesList();
                m_bIgnoreInput = false;



                if(m_saveDetails!=nullptr)

                {

                    for(unsigned int i = 0; i < m_iSaveDetailsCount; ++i)

                    {

                        if(m_saveDetails[i].pbThumbnailData!=nullptr)

                        {

                            delete m_saveDetails[i].pbThumbnailData;

                        }

                    }

                    delete m_saveDetails;

                }

                m_saveDetails = new SaveListDetails[m_pSaveDetails->iSaveC];



                m_iSaveDetailsCount = m_pSaveDetails->iSaveC;

#ifdef _WINDOWS64

                // Build sorted index array (newest-first by filename timestamp YYYYMMDDHHMMSS)

                int *sortedIdx = new int[m_pSaveDetails->iSaveC];

                for (int si = 0; si < (int)m_pSaveDetails->iSaveC; ++si) sortedIdx[si] = si;

                for (int si = 1; si < (int)m_pSaveDetails->iSaveC; ++si)

                {

                    int key = sortedIdx[si];

                    int sj = si - 1;

                    while (sj >= 0 && strcmp(m_pSaveDetails->SaveInfoA[sortedIdx[sj]].UTF8SaveFilename, m_pSaveDetails->SaveInfoA[key].UTF8SaveFilename) < 0)

                    {

                        sortedIdx[sj + 1] = sortedIdx[sj];

                        --sj;

                    }

                    sortedIdx[sj + 1] = key;

                }

#endif

                for(unsigned int i = 0; i < m_pSaveDetails->iSaveC; ++i)

                {

#if defined(_XBOX_ONE)

                    m_spaceIndicatorSaves.addSave(m_pSaveDetails->SaveInfoA[i].totalSize);

#elif defined(_WINDOWS64)

                    int origIdx = sortedIdx[i];

                    wchar_t wFilename[MAX_SAVEFILENAME_LENGTH];

                    ZeroMemory(wFilename, sizeof(wFilename));

                    mbstowcs(wFilename, m_pSaveDetails->SaveInfoA[origIdx].UTF8SaveFilename, MAX_SAVEFILENAME_LENGTH - 1);

                    wstring filePath = wstring(L"Windows64\\GameHDD\\") + wstring(wFilename) + wstring(L"\\saveData.ms");



                    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);

                    DWORD fileSize = 0;



                    if (hFile != INVALID_HANDLE_VALUE) {

                        fileSize = GetFileSize(hFile, nullptr);

                        if (fileSize < 12 || fileSize == INVALID_FILE_SIZE) fileSize = 0;

                        CloseHandle(hFile);

                    }

                    m_spaceIndicatorSaves.addSave(fileSize);

#elif defined(__ORBIS__)

                    m_spaceIndicatorSaves.addSave(m_pSaveDetails->SaveInfoA[i].blocksUsed * (32 * 1024) );

#endif

#ifdef _DURANGO

                    m_buttonListSaves.addItem(m_pSaveDetails->SaveInfoA[i].UTF16SaveTitle, L"");



                    m_saveDetails[i].saveId = i;

                    memcpy(m_saveDetails[i].UTF16SaveName, m_pSaveDetails->SaveInfoA[i].UTF16SaveTitle, 128);

                    memcpy(m_saveDetails[i].UTF16SaveFilename, m_pSaveDetails->SaveInfoA[i].UTF16SaveFilename, MAX_SAVEFILENAME_LENGTH);

#else

#ifdef _WINDOWS64

                    {

                        wstring levelName = ReadLevelNameFromSaveFile(filePath);



                        if (!levelName.empty())

                        {

                            m_buttonListSaves.addItem(levelName, wstring(L""));

                            wcstombs(m_saveDetails[i].UTF8SaveName, levelName.c_str(), 127);

                            m_saveDetails[i].UTF8SaveName[127] = '\0';

                        }

                        else

                        {

                            const char *saveTitle = m_pSaveDetails->SaveInfoA[origIdx].UTF8SaveTitle;

                            if(IsGeneratedSaveTitle(saveTitle))

                            {

                                wstring fallbackName = GetGeneratedSaveFallbackName(m_pSaveDetails->SaveInfoA[origIdx]);

                                m_buttonListSaves.addItem(fallbackName, wstring(L""));

                                wcstombs(m_saveDetails[i].UTF8SaveName, fallbackName.c_str(), 127);

                                m_saveDetails[i].UTF8SaveName[127] = '\0';

                            }

                            else

                            {

                                m_buttonListSaves.addItem(saveTitle, L"");

                                memcpy(m_saveDetails[i].UTF8SaveName, saveTitle, 128);

                            }

                        }

                        m_saveDetails[i].saveId = origIdx;

                        memcpy(m_saveDetails[i].UTF8SaveFilename, m_pSaveDetails->SaveInfoA[origIdx].UTF8SaveFilename, MAX_SAVEFILENAME_LENGTH);

                        if(m_pSaveDetails->SaveInfoA[origIdx].thumbnailData != nullptr && m_pSaveDetails->SaveInfoA[origIdx].metaData.thumbnailSize > 0)
                        {
                            m_saveDetails[i].dwThumbnailSize = m_pSaveDetails->SaveInfoA[origIdx].metaData.thumbnailSize;
                            m_saveDetails[i].pbThumbnailData = new BYTE[m_saveDetails[i].dwThumbnailSize];
                            memcpy(m_saveDetails[i].pbThumbnailData, m_pSaveDetails->SaveInfoA[origIdx].thumbnailData, m_saveDetails[i].dwThumbnailSize);
                        }

                    }

#else

                    const char *saveTitle = m_pSaveDetails->SaveInfoA[i].UTF8SaveTitle;

                    if(IsGeneratedSaveTitle(saveTitle))

                    {                        wstring fallbackName = GetGeneratedSaveFallbackName(m_pSaveDetails->SaveInfoA[i]);

                        m_buttonListSaves.addItem(fallbackName, wstring(L""));

                        wcstombs(m_saveDetails[i].UTF8SaveName, fallbackName.c_str(), 127);

                        m_saveDetails[i].UTF8SaveName[127] = '\0';

                    }

                    else

                    {

                        m_buttonListSaves.addItem(saveTitle, L"");

                        memcpy(m_saveDetails[i].UTF8SaveName, saveTitle, 128);

                    }

                    m_saveDetails[i].saveId = i;

                    memcpy(m_saveDetails[i].UTF8SaveFilename, m_pSaveDetails->SaveInfoA[i].UTF8SaveFilename, MAX_SAVEFILENAME_LENGTH);

#endif

#endif

                }

#ifdef _WINDOWS64

                delete[] sortedIdx;

#endif

                m_controlSavesTimer.setVisible( false );
                updateTooltips();



                // set focus on the first button



			}

        }



        if(!m_bExitScene && m_bSavesDisplayed && !m_bRetrievingSaveThumbnails && !m_bAllLoaded)

        {

            if( m_iRequestingThumbnailId < m_buttonListSaves.getItemCount())

            {

#ifdef _WINDOWS64
                if(m_saveDetails[m_iRequestingThumbnailId].pbThumbnailData != nullptr && m_saveDetails[m_iRequestingThumbnailId].dwThumbnailSize > 0)
                {
                    app.DebugPrintf("Using preloaded save thumbnail\n");
                    m_bSaveThumbnailReady = true;
                }
                else
#endif
                {
                    m_bRetrievingSaveThumbnails = true;

                    app.DebugPrintf("Requesting the first thumbnail\n");

                    // set the save to load

                    PSAVE_DETAILS pSaveDetails=StorageManager.ReturnSavesInfo();

#ifdef _WINDOWS64

                C4JStorage::ESaveGameState eLoadStatus=StorageManager.LoadSaveDataThumbnail(&pSaveDetails->SaveInfoA[m_saveDetails[m_iRequestingThumbnailId].saveId],&LoadSaveDataThumbnailReturned,this);

#else

                C4JStorage::ESaveGameState eLoadStatus=StorageManager.LoadSaveDataThumbnail(&pSaveDetails->SaveInfoA[(int)m_iRequestingThumbnailId],&LoadSaveDataThumbnailReturned,this);

#endif



                if(eLoadStatus!=C4JStorage::ESaveGame_GetSaveThumbnail)

                {

                    // something went wrong

                    m_bRetrievingSaveThumbnails=false;

                    m_bAllLoaded = true;

                }

                }

            }

        }

        else if (m_bSavesDisplayed && m_bSaveThumbnailReady)

        {

            m_bSaveThumbnailReady = false;



            // check we're not waiting to exit the scene

            if(!m_bExitScene)

            {

                // convert to utf16

                uint16_t u16Message[MAX_SAVEFILENAME_LENGTH];

#ifdef _DURANGO

                // Already utf16 on durango

                memcpy(u16Message, m_saveDetails[m_iRequestingThumbnailId].UTF16SaveFilename, MAX_SAVEFILENAME_LENGTH);

#elif defined(_WINDOWS64)

                int result = ::MultiByteToWideChar(

                    CP_UTF8,                // convert from UTF-8

                    MB_ERR_INVALID_CHARS,   // error on invalid chars

                    m_saveDetails[m_iRequestingThumbnailId].UTF8SaveFilename,            // source UTF-8 string

                    MAX_SAVEFILENAME_LENGTH,                 // total length of source UTF-8 string,

                    // in CHAR's (= bytes), including end-of-string \0

                    (wchar_t *)u16Message,               // destination buffer

                    MAX_SAVEFILENAME_LENGTH                // size of destination buffer, in WCHAR's

                    );

#else

#ifdef __PS3

                size_t srcmax,dstmax;

#else

                uint32_t srcmax,dstmax;

                uint32_t srclen,dstlen;

#endif

                srcmax=MAX_SAVEFILENAME_LENGTH;

                dstmax=MAX_SAVEFILENAME_LENGTH;



#if defined(__PS3__)

                L10nResult lres= UTF8stoUTF16s((uint8_t *)m_saveDetails[m_iRequestingThumbnailId].UTF8SaveFilename,&srcmax,u16Message,&dstmax);

#else

                SceCesUcsContext context;

                sceCesUcsContextInit(&context);



                sceCesUtf8StrToUtf16Str(&context, (uint8_t *)m_saveDetails[m_iRequestingThumbnailId].UTF8SaveFilename,srcmax,&srclen,u16Message,dstmax,&dstlen);

#endif

#endif

                const int saveId = m_saveDetails[m_iRequestingThumbnailId].saveId;

                if( m_saveDetails[m_iRequestingThumbnailId].pbThumbnailData )

                {
                    wchar_t textureName[64];
                    swprintf(textureName,64,L"loadsave_large_%08x",saveId);
                    registerSubstitutionTexture(textureName,m_saveDetails[m_iRequestingThumbnailId].pbThumbnailData,m_saveDetails[m_iRequestingThumbnailId].dwThumbnailSize);
                    wcscpy((wchar_t *)u16Message, textureName);

                }

                m_buttonListSaves.setTextureName(m_iRequestingThumbnailId, (wchar_t *)u16Message);



                ++m_iRequestingThumbnailId;

                if( m_iRequestingThumbnailId < m_buttonListSaves.getItemCount())

                {

#ifdef _WINDOWS64
                    if(m_saveDetails[m_iRequestingThumbnailId].pbThumbnailData != nullptr && m_saveDetails[m_iRequestingThumbnailId].dwThumbnailSize > 0)
                    {
                        app.DebugPrintf("Using preloaded save thumbnail\n");
                        m_bRetrievingSaveThumbnails = false;
                        m_bSaveThumbnailReady = true;
                    }
                    else
#endif
                    {
                        app.DebugPrintf("Requesting another thumbnail\n");

                        // set the save to load

                        PSAVE_DETAILS pSaveDetails=StorageManager.ReturnSavesInfo();

#ifdef _WINDOWS64

                    C4JStorage::ESaveGameState eLoadStatus=StorageManager.LoadSaveDataThumbnail(&pSaveDetails->SaveInfoA[m_saveDetails[m_iRequestingThumbnailId].saveId],&LoadSaveDataThumbnailReturned,this);

#else

                    C4JStorage::ESaveGameState eLoadStatus=StorageManager.LoadSaveDataThumbnail(&pSaveDetails->SaveInfoA[(int)m_iRequestingThumbnailId],&LoadSaveDataThumbnailReturned,this);

#endif

                    if(eLoadStatus!=C4JStorage::ESaveGame_GetSaveThumbnail)

                    {

                        // something went wrong

                        m_bRetrievingSaveThumbnails=false;

                        m_bAllLoaded = true;

                    }

                    }

                }

                else

                {

                    m_bRetrievingSaveThumbnails = false;

                    m_bAllLoaded = true;

                }

            }

            else

            {

                // stop retrieving thumbnails, and exit

                m_bRetrievingSaveThumbnails = false;

            }

        }

    }



    switch(m_iState)

    {

    case e_SavesIdle:

        break;

    case e_SavesRepopulate:

        m_bIgnoreInput = false;

        m_iState=e_SavesIdle;

        m_bAllLoaded=false;

        m_bRetrievingSaveThumbnails=false;

        m_iRequestingThumbnailId = 0;

        GetSaveInfo();

        break;

	case e_SavesRepopulateAfterMashupHide:

        m_bIgnoreInput = false;

        m_iRequestingThumbnailId = 0;

        m_bAllLoaded=false;

        m_bRetrievingSaveThumbnails=false;

        m_bSavesDisplayed=false;

        m_iSaveInfoC=0;

        m_buttonListSaves.clearList();

        GetSaveInfo();

        m_iState=e_SavesIdle;

		break;

    case e_SavesRepopulateAfterDelete:

	case e_SavesRepopulateAfterTransferDownload:

        m_bIgnoreInput = false;

        m_iRequestingThumbnailId = 0;

        m_bAllLoaded=false;

        m_bRetrievingSaveThumbnails=false;

        m_bSavesDisplayed=false;

        m_iSaveInfoC=0;

        m_buttonListSaves.clearList();

        StorageManager.ClearSavesInfo();

        GetSaveInfo();

        m_iState=e_SavesIdle;

        break;

    }

#else

    if(!m_bSavesDisplayed)

    {

        AddDefaultButtons();

        m_bSavesDisplayed=true;

        m_controlSavesTimer.setVisible( false );

    }

#endif



#ifdef _XBOX_ONE

	if(g_NetworkManager.ShouldMessageForFullSession())

	{

		UINT uiIDA[1];

		uiIDA[0]=IDS_CONFIRM_OK;

		ui.RequestErrorMessage( IDS_CONNECTION_FAILED, IDS_IN_PARTY_SESSION_FULL, uiIDA,1,ProfileManager.GetPrimaryPad());

	}

#endif



    // SAVE TRANSFERS

#ifdef __ORBIS__

	// check the status of the PSPlus common dialog

	switch (sceNpCommerceDialogUpdateStatus())

	{

	case SCE_COMMON_DIALOG_STATUS_FINISHED:

		{

			SceNpCommerceDialogResult Result;

			sceNpCommerceDialogGetResult(&Result);

			sceNpCommerceDialogTerminate();



			if(Result.authorized)

			{

				// they just became a PSPlus member

				ProfileManager.PsPlusUpdate(ProfileManager.GetPrimaryPad(), &Result);



			}

			else

			{



			}



			// 4J-JEV: Fix for PS4 #5148 - [ONLINE] If the user attempts to join a game when they do not have Playstation Plus, the title will lose all functionality.

			m_bIgnoreInput = false;

		}

		break;

	default:

		break;

	}

#endif



}


void UIScene_LoadCreateJoinMenu::GetSaveInfo()

{

    unsigned int uiSaveC=0;



    // This will return with the number retrieved in uiSaveC



    if(app.DebugSettingsOn() && app.GetLoadSavesFromFolderEnabled())

    {

#ifdef __ORBIS__

		// We need to make sure this is non-null so that we have an idea of free space

        m_pSaveDetails=StorageManager.ReturnSavesInfo();

        if(m_pSaveDetails==nullptr)

        {

            C4JStorage::ESaveGameState eSGIStatus= StorageManager.GetSavesInfo(m_iPad,nullptr,this,"save");

        }

#endif



        uiSaveC = 0;

#ifdef _XBOX

        File savesDir(L"GAME:\\Saves");

#else

        File savesDir(L"Saves");

#endif

        if( savesDir.exists() )

        {

            m_saves = savesDir.listFiles();

            uiSaveC = static_cast<unsigned int>(m_saves->size());

        }

        // add the New Game and Tutorial after the saves list is retrieved, if there are any saves



        // Add two for New Game and Tutorial

        unsigned int listItems = uiSaveC;



        AddDefaultButtons();



        for(unsigned int i=0;i<listItems;i++)

        {



            wstring wName = m_saves->at(i)->getName();

            wchar_t *name = new wchar_t[wName.size()+1];

            for(unsigned int j = 0; j < wName.size(); ++j)

            {

                name[j] = wName[j];

            }

            name[wName.size()] = 0;

            m_buttonListSaves.addItem(name,L"");

        }

        m_bSavesDisplayed = true;

        m_bAllLoaded = true;

        m_bIgnoreInput = false;

    }

    else

    {

        // clear the saves list

        m_bSavesDisplayed = false; // we're blocking the exit from this scene until complete

        m_buttonListSaves.clearList();

        m_iSaveInfoC=0;

        m_controlSavesTimer.setVisible(true);



        m_pSaveDetails=StorageManager.ReturnSavesInfo();

        if(m_pSaveDetails==nullptr)

        {

            C4JStorage::ESaveGameState eSGIStatus= StorageManager.GetSavesInfo(m_iPad, nullptr,this,"save");

        }



#if TO_BE_IMPLEMENTED

        if(eSGIStatus==C4JStorage::ESGIStatus_NoSaves)

        {

            uiSaveC=0;

            m_controlSavesTimer.setVisible( false );

            m_SavesList.SetEnable(TRUE);

        }

#endif

    }



    return;

}



void UIScene_LoadCreateJoinMenu::AddDefaultButtons()

{
    // hidden pack entries need a full rebuild or the list can duplicate
    // create list uses its own index path do not reuse load index here
    // keep all mashup worlds available in create on legacy evolved
    app.EnableMashupPackWorlds(m_iPad);

    m_iDefaultButtonsC = 0;

	m_iMashUpButtonsC=0;

    m_iNewGameListIndex = 0;

    m_buttonListNewGames.clearList();

	m_generators.clear();



	m_buttonListNewGames.addItem(app.GetString(IDS_CREATE_NEW_WORLD));

#ifdef _WINDOWS64
    TrySetButtonListIcon(
        m_buttonListNewGames,
        m_buttonListNewGames.getItemCount() - 1,
        L"CreateWorldIcon.png",
        L"CreateWorldIcon");
#endif

    m_iDefaultButtonsC++;



    int i = 0;



    for ( LevelGenerationOptions *levelGen : *app.getLevelGenerators() )

    {

        // retrieve the save icon from the texture pack, if there is one

        unsigned int uiTexturePackID=levelGen->getRequiredTexturePackId();



		if(uiTexturePackID!=0)

		{

			unsigned int uiMashUpWorldsBitmask=app.GetMashupPackWorlds(m_iPad);



			if((uiMashUpWorldsBitmask & (1<<(uiTexturePackID-1024)))==0)

			{

				// this world is hidden, so skip

				continue;

			}

		}



		// 4J-JEV: For debug. Ignore worlds with no name.

		LPCWSTR wstr = levelGen->getWorldName();

		m_buttonListNewGames.addItem( wstr );

		m_generators.push_back(levelGen);



#ifdef _WINDOWS64
        if(levelGen->isTutorial())
        {
            TrySetButtonListIcon(
                m_buttonListNewGames,
                m_buttonListNewGames.getItemCount() - 1,
                L"TutorialIcon.png",
                L"TutorialIcon");
        }
#endif

        if(uiTexturePackID!=0)

        {

			// increment the count of the mash-up pack worlds in the save list

			m_iMashUpButtonsC++;

            TexturePack *tp = Minecraft::GetInstance()->skins->getTexturePackById(levelGen->getRequiredTexturePackId());

            DWORD dwImageBytes;

            PBYTE pbImageData = tp->getPackIcon(dwImageBytes);



            if(dwImageBytes > 0 && pbImageData)

            {

                wchar_t imageName[64];
                const int slotIndex = m_buttonListNewGames.getItemCount() - 1;

                swprintf(imageName,64,L"tpack_small_slot%08x",slotIndex);

                registerSubstitutionTexture(imageName, pbImageData, dwImageBytes);

                m_buttonListNewGames.setTextureName( m_buttonListNewGames.getItemCount() - 1, imageName );

            }

        }



        ++i;

    }

    m_iDefaultButtonsC += i;

}



void UIScene_LoadCreateJoinMenu::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled)

{

    if(m_bIgnoreInput) return;



    // if we're retrieving save info, ignore key presses

    if(!m_bSavesDisplayed) return;



    ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

    auto openSaveOptions = [&]() -> bool
    {
        if(!pressed || repeat || !ProfileManager.IsFullVersion() || m_activeTab != eTab_Load)
            return false;

        if((m_pSaveDetails != nullptr) &&
           (m_pSaveDetails->iSaveC > 0) &&
           (m_buttonListSaves.getItemCount() > 0) &&
           (m_iSaveListIndex >= 0) &&
           (m_iSaveListIndex < m_buttonListSaves.getItemCount()))
        {
            m_bIgnoreInput = true;

            if(StorageManager.EnoughSpaceForAMinSaveGame() && !StorageManager.GetSaveDisabled())
            {
                UINT uiIDA[3];
                uiIDA[0]=IDS_CONFIRM_CANCEL;
                uiIDA[1]=IDS_TITLE_RENAMESAVE;
                uiIDA[2]=IDS_TOOLTIPS_DELETESAVE;
                ui.RequestAlertMessage(IDS_TOOLTIPS_SAVEOPTIONS, IDS_TEXT_SAVEOPTIONS, uiIDA, 3, iPad,&UIScene_LoadCreateJoinMenu::SaveOptionsDialogReturned,this);
            }
            else
            {
                UINT uiIDA[2];
                uiIDA[0]=IDS_CONFIRM_CANCEL;
                uiIDA[1]=IDS_CONFIRM_OK;
                ui.RequestAlertMessage(IDS_TOOLTIPS_DELETESAVE, IDS_TEXT_DELETE_SAVE, uiIDA, 2, iPad,&UIScene_LoadCreateJoinMenu::DeleteSaveDialogReturned,this);
            }

            ui.PlayUISFX(eSFX_Press);
            return true;
        }

        return false;
    };

    auto hideFocusedMashupWorld = [&]() -> bool
    {
        if(!pressed || repeat || !ProfileManager.IsFullVersion() || !DoesMashUpWorldHaveFocus())
            return false;

        const int lGenID = m_iNewGameListIndex - m_iDefaultButtonsC;
        if((lGenID < 0) || (lGenID >= static_cast<int>(m_generators.size())))
            return false;

        LevelGenerationOptions *levelGen = m_generators.at(lGenID);
        if(!levelGen || levelGen->isTutorial() || !levelGen->requiresTexturePack())
            return false;

        const int oldIndex = m_iNewGameListIndex;
        m_bIgnoreInput = true;
        app.HideMashupPackWorld(m_iPad, levelGen->getRequiredTexturePackId());
        AddDefaultButtons();

        if(m_buttonListNewGames.getItemCount() <= 0)
        {
            m_iNewGameListIndex = 0;
        }
        else if(oldIndex >= m_buttonListNewGames.getItemCount())
        {
            m_iNewGameListIndex = m_buttonListNewGames.getItemCount() - 1;
        }
        else if(oldIndex >= 0)
        {
            m_iNewGameListIndex = oldIndex;
        }

        if(m_buttonListNewGames.getItemCount() > 0)
        {
            m_buttonListNewGames.setCurrentSelection(m_iNewGameListIndex);
        }

        updateTooltips();
        m_iState = e_SavesIdle;
        ui.PlayUISFX(eSFX_Press);
        m_bIgnoreInput = false;
        return true;
    };

    switch(key)

    {

    case ACTION_MENU_CANCEL:

        if(pressed)

        {

#if defined(__PS3__) || defined(__ORBIS__) || defined(__PSVITA__)

            m_bExitScene=true;

#else

            navigateBack();

#endif

            handled = true;

        }

        break;

    case ACTION_MENU_X:

#if TO_BE_IMPLEMENTED

        // Change device

        // Fix for #12531 - TCR 001: BAS Game Stability: When a player selects to change a storage

        // device, and repeatedly backs out of the SD screen, disconnects from LIVE, and then selects a SD, the title crashes.

        m_bIgnoreInput=true;

        StorageManager.SetSaveDevice(&CScene_MultiGameJoinLoad::DeviceSelectReturned,this,true);

        ui.PlayUISFX(eSFX_Press);

#endif

        // Save Transfer

#ifdef _XBOX_ONE

		if(ProfileManager.IsSignedInLive( m_iPad ))

		{

			UIScene_LoadCreateJoinMenu::s_ulFileSize=0;

			LaunchSaveTransfer();

		}

#endif

#ifdef SONY_REMOTE_STORAGE_DOWNLOAD

		{

			bool bSignedInLive = ProfileManager.IsSignedInLive(iPad);

			if(bSignedInLive)

			{

				LaunchSaveTransfer();

			}

		}

#endif

#ifdef _WINDOWS64

		// Right click on a save opens save options. In Create, it hides mash-up worlds.

        if(hideFocusedMashupWorld())
        {
            handled = true;
        }
		else if(openSaveOptions())

		{
            handled = true;
		}

#endif

        break;

    case ACTION_MENU_Y:

#ifdef _WINDOWS64
        if(hideFocusedMashupWorld())
        {
            handled = true;
            break;
        }

        if(openSaveOptions())
        {
            handled = true;
            break;
        }
#endif

#if defined(__PS3__) || defined(__PSVITA__) || defined(__ORBIS__)

		m_eAction = eAction_ViewInvites;

        if(pressed && iPad == ProfileManager.GetPrimaryPad())

        {

#ifdef __ORBIS__

			// Check if PSN is unavailable because of age restriction

			int npAvailability = ProfileManager.getNPAvailability(iPad);

			if (npAvailability == SCE_NP_ERROR_AGE_RESTRICTION)

			{

				UINT uiIDA[1];

				uiIDA[0] = IDS_OK;

				ui.RequestErrorMessage(IDS_ONLINE_SERVICE_TITLE, IDS_CONTENT_RESTRICTION, uiIDA, 1, iPad);



				break;

			}

#endif



            // are we offline?

            if(!ProfileManager.IsSignedInLive(iPad))

            {

                // get them to sign in to online

                UINT uiIDA[2];

                uiIDA[0]=IDS_PRO_NOTONLINE_ACCEPT;

                uiIDA[1]=IDS_PRO_NOTONLINE_DECLINE;

                ui.RequestAlertMessage(IDS_PRO_NOTONLINE_TITLE, IDS_PRO_NOTONLINE_TEXT, uiIDA, 2, ProfileManager.GetPrimaryPad(), &UIScene_LoadCreateJoinMenu::MustSignInReturnedPSN, this);

            }

            else

            {

#ifdef __ORBIS__

                SQRNetworkManager_Orbis::RecvInviteGUI();

#elif defined __PSVITA__

                SQRNetworkManager_Vita::RecvInviteGUI();

#else

                int ret = sceNpBasicRecvMessageCustom(SCE_NP_BASIC_MESSAGE_MAIN_TYPE_INVITE, SCE_NP_BASIC_RECV_MESSAGE_OPTIONS_INCLUDE_BOOTABLE, SYS_MEMORY_CONTAINER_ID_INVALID);

                app.DebugPrintf("sceNpBasicRecvMessageCustom return %d ( %08x )\n", ret, ret);

#endif

            }

        }

#elif defined(_DURANGO)

        if(getControlFocus() == eControl_GamesList && m_buttonListGames.getItemCount() > 0)

        {

            DWORD nIndex = m_buttonListGames.getCurrentSelection();

            FriendSessionInfo *pSelectedSession = m_currentSessions->at( nIndex );



            PlayerUID uid = pSelectedSession->searchResult.m_playerXuids[0];

            if( uid != INVALID_XUID ) ProfileManager.ShowProfileCard(ProfileManager.GetLockedProfile(),uid);

            ui.PlayUISFX(eSFX_Press);

        }

#endif // __PS3__ || __ORBIS__

        break;



    case ACTION_MENU_RIGHT_SCROLL:

        if(pressed && !repeat)

        {

            ui.PlayUISFX(eSFX_Focus);

            if (m_activeTab == eTab_Load) SetActiveTab(eTab_Create, true);

            else if (m_activeTab == eTab_Create) SetActiveTab(eTab_Join, true);

            else SetActiveTab(eTab_Load, true);

            handled = true;

            break;

        }


        else if(hideFocusedMashupWorld())

        {
            handled = true;

        }

        break;



    case ACTION_MENU_LEFT_SCROLL:

        if(pressed && !repeat)

        {

            ui.PlayUISFX(eSFX_Focus);

            if (m_activeTab == eTab_Load) SetActiveTab(eTab_Join, true);

            else if (m_activeTab == eTab_Create) SetActiveTab(eTab_Load, true);

            else SetActiveTab(eTab_Create, true);

            handled = true;

            break;

        }

#ifdef _XBOX

        if( m_bInParty )

        {

            m_bShowingPartyGamesOnly = !m_bShowingPartyGamesOnly;

            UpdateGamesList();

            CXuiSceneBase::PlayUISFX(eSFX_Press);

        }

#endif

        break;



    case ACTION_MENU_LEFT:

        if(pressed && !repeat)
        {
            ui.PlayUISFX(eSFX_Focus);
            if (m_activeTab == eTab_Load) SetActiveTab(eTab_Join, true);
            else if (m_activeTab == eTab_Create) SetActiveTab(eTab_Load, true);
            else SetActiveTab(eTab_Create, true);
        }

        handled = true;

        break;

    case ACTION_MENU_RIGHT:

        if(pressed && !repeat)
        {
            ui.PlayUISFX(eSFX_Focus);
            if (m_activeTab == eTab_Load) SetActiveTab(eTab_Create, true);
            else if (m_activeTab == eTab_Create) SetActiveTab(eTab_Join, true);
            else SetActiveTab(eTab_Load, true);
        }

        handled = true;

        break;





    case ACTION_MENU_OK:

#ifdef __ORBIS__

    case ACTION_MENU_TOUCHPAD_PRESS:

#endif

    case ACTION_MENU_UP:

    case ACTION_MENU_DOWN:

    case ACTION_MENU_PAGEUP:

    case ACTION_MENU_PAGEDOWN:

        sendInputToMovie(key, repeat, pressed, released);

        handled = true;

        break;

    case ACTION_MENU_OTHER_STICK_UP:

        sendInputToMovie(ACTION_MENU_UP, repeat, pressed, released);

        handled = true;

        break;

    case ACTION_MENU_OTHER_STICK_DOWN:

        sendInputToMovie(ACTION_MENU_DOWN, repeat, pressed, released);

        handled = true;

        break;

    }

}



int UIScene_LoadCreateJoinMenu::KeyboardCompleteWorldNameCallback(LPVOID lpParam,bool bRes)

{

    // 4J HEG - No reason to set value if keyboard was cancelled

    UIScene_LoadCreateJoinMenu *pClass=static_cast<UIScene_LoadCreateJoinMenu *>(lpParam);

	pClass->m_bIgnoreInput=false;

    if (bRes)

    {

        uint16_t ui16Text[128];

        ZeroMemory(ui16Text, 128 * sizeof(uint16_t) );

#ifdef _WINDOWS64

        Win64_GetKeyboardText(ui16Text, 128);

#else

        InputManager.GetText(ui16Text);

#endif



        // check the name is valid

        if(ui16Text[0]!=0)

        {

#if (defined __PS3__ || defined __ORBIS__ || defined _DURANGO  || defined(__PSVITA__))

            // open the save and overwrite the metadata

            StorageManager.RenameSaveData(pClass->m_iSaveListIndex, ui16Text,&UIScene_LoadCreateJoinMenu::RenameSaveDataReturned,pClass);

#elif defined(_WINDOWS64)

            {

                int listPos = pClass->m_iSaveListIndex;



                // Convert the ui16Text input to a wide string

                wchar_t wNewName[128] = {};

                for (int k = 0; k < 127 && ui16Text[k]; k++)

                    wNewName[k] = static_cast<wchar_t>(ui16Text[k]);



                // Convert to narrow for storage and in-memory update

                char narrowName[128] = {};

                wcstombs(narrowName, wNewName, 127);



                // Build the sidecar path: Windows64\GameHDD\{folder}\worldname.txt

                wchar_t wFilename[MAX_SAVEFILENAME_LENGTH] = {};

                mbstowcs(wFilename, pClass->m_saveDetails[listPos].UTF8SaveFilename, MAX_SAVEFILENAME_LENGTH - 1);

                wstring sidecarPath = wstring(L"Windows64\\GameHDD\\") + wstring(wFilename) + wstring(L"\\worldname.txt");



                FILE *fw = nullptr;

                if (_wfopen_s(&fw, sidecarPath.c_str(), L"w") == 0 && fw)

                {

                    fputs(narrowName, fw);

                    fclose(fw);

                }



                // Update the in-memory display name so the list reflects it immediately

                strncpy_s(pClass->m_saveDetails[listPos].UTF8SaveName, narrowName, 127);

                pClass->m_saveDetails[listPos].UTF8SaveName[127] = '\0';



                // Reuse the existing callback to trigger the list repopulate

                UIScene_LoadCreateJoinMenu::RenameSaveDataReturned(pClass, true);

            }

#endif

        }

        else

        {

            pClass->m_bIgnoreInput=false;

            pClass->updateTooltips();

        }

    }

    else

    {

        pClass->m_bIgnoreInput=false;

        pClass->updateTooltips();

    }





    return 0;

}

void UIScene_LoadCreateJoinMenu::handleInitFocus(F64 controlId, F64 childId)

{

    app.DebugPrintf(app.USER_SR, "UIScene_LoadCreateJoinMenu::handleInitFocus - %d , %d\n", static_cast<int>(controlId), static_cast<int>(childId));

    SetActiveTab(m_activeTab, false);
    m_bPendingSaveSizeBarRefresh = true;
    m_bPendingJoinTabAvailabilityRefresh = true;

}

void UIScene_LoadCreateJoinMenu::handleFocusChange(F64 controlId, F64 childId)

{
    const int visibleRows = 7;

    if (m_bRebuildingJoinVisual && static_cast<int>(controlId) == eControl_GamesList)
    {
        return;
    }

    if (static_cast<int>(controlId) == eControl_GamesList && m_activeTab != eTab_Join)
    {
        return;
    }

    if (static_cast<int>(controlId) == eControl_NewGamesList && m_activeTab != eTab_Create)
    {
        return;
    }

    if (static_cast<int>(controlId) == eControl_SavesList && m_activeTab != eTab_Load)
    {
        return;
    }



    const ELoadCreateJoinTab oldTab = m_activeTab;
    switch(static_cast<int>(controlId))

    {

    case eControl_TabLoad:

        m_activeTab = eTab_Load;

        break;

    case eControl_TabCreate:

        m_activeTab = eTab_Create;

        break;

    case eControl_TabJoin:

        m_activeTab = eTab_Join;

        break;

    case eControl_GamesList:

        m_activeTab = eTab_Join;

        m_iGameListIndex = childId;

#ifdef _WINDOWS64

        m_iGameListIndex -= 1;

#endif

        m_buttonListGames.updateChildFocus(static_cast<int>(childId));

        if (childId < m_hoverBaseIndexJoin)
            m_hoverBaseIndexJoin = static_cast<int>(childId);
        else if (childId >= m_hoverBaseIndexJoin + visibleRows)
            m_hoverBaseIndexJoin = static_cast<int>(childId) - visibleRows + 1;

        {
            const int maxJoinBase = (m_buttonListGames.getItemCount() - visibleRows > 0) ? (m_buttonListGames.getItemCount() - visibleRows) : 0;
            if (m_hoverBaseIndexJoin < 0)
                m_hoverBaseIndexJoin = 0;
            else if (m_hoverBaseIndexJoin > maxJoinBase)
                m_hoverBaseIndexJoin = maxJoinBase;
        }

        break;

    case eControl_SavesList:

        m_activeTab = eTab_Load;

        m_iSaveListIndex = childId;

        m_bUpdateSaveSize = true;

        if (childId < m_hoverBaseIndexLoad)
            m_hoverBaseIndexLoad = static_cast<int>(childId);
        else if (childId >= m_hoverBaseIndexLoad + visibleRows)
            m_hoverBaseIndexLoad = static_cast<int>(childId) - visibleRows + 1;

        {
            const int maxLoadBase = (m_buttonListSaves.getItemCount() - visibleRows > 0) ? (m_buttonListSaves.getItemCount() - visibleRows) : 0;
            if (m_hoverBaseIndexLoad < 0)
                m_hoverBaseIndexLoad = 0;
            else if (m_hoverBaseIndexLoad > maxLoadBase)
                m_hoverBaseIndexLoad = maxLoadBase;
        }

        break;

    case eControl_NewGamesList:

        m_activeTab = eTab_Create;

        m_iNewGameListIndex = childId;

        m_buttonListNewGames.updateChildFocus(static_cast<int>(childId));

        if (childId < m_hoverBaseIndexCreate)
            m_hoverBaseIndexCreate = static_cast<int>(childId);
        else if (childId >= m_hoverBaseIndexCreate + visibleRows)
            m_hoverBaseIndexCreate = static_cast<int>(childId) - visibleRows + 1;

        {
            const int maxCreateBase = (m_buttonListNewGames.getItemCount() - visibleRows > 0) ? (m_buttonListNewGames.getItemCount() - visibleRows) : 0;
            if (m_hoverBaseIndexCreate < 0)
                m_hoverBaseIndexCreate = 0;
            else if (m_hoverBaseIndexCreate > maxCreateBase)
                m_hoverBaseIndexCreate = maxCreateBase;
        }

        break;

    };

    updateTooltips();
    if (m_activeTab != oldTab)
    {
        SetActiveTab(m_activeTab, false);
        m_bPendingSaveSizeBarRefresh = true;
        m_bPendingJoinTabAvailabilityRefresh = true;
    }

}

void UIScene_LoadCreateJoinMenu::handlePress(F64 controlId, F64 childId)

{
    switch(static_cast<int>(controlId))

    {

    case eControl_TabLoad:

        ui.PlayUISFX(eSFX_Press);

        SetActiveTab(eTab_Load, true);

        return;



    case eControl_TabCreate:

        ui.PlayUISFX(eSFX_Press);

        SetActiveTab(eTab_Create, true);

        return;



    case eControl_TabJoin:

        ui.PlayUISFX(eSFX_Press);

        SetActiveTab(eTab_Join, true);

        return;



    case eControl_NewGamesList:

        {

            m_bIgnoreInput = true;

            ui.PlayUISFX(eSFX_Press);



            if(static_cast<int>(childId) == 0)

            {
                app.SetTutorialMode(false);

                m_controlJoinTimer.setVisible(false);

                app.SetCorruptSaveDeleted(false);



                CreateWorldMenuInitData *params = new CreateWorldMenuInitData();

                params->iPad = m_iPad;

                ui.NavigateToScene(m_iPad, eUIScene_CreateWorldMenu, (void *)params);

            }

            else

            {

                int lGenID = static_cast<int>(childId) - 1;

                if(lGenID < static_cast<int>(m_generators.size()))

                {

                    LevelGenerationOptions *levelGen = m_generators.at(lGenID);

                    app.SetTutorialMode(levelGen->isTutorial());

                    app.SetAutosaveTimerTime();



                    if(levelGen->isTutorial())

                    {

                        LoadLevelGen(levelGen);

                    }

                    else

                    {
                        LoadMenuInitData *params = new LoadMenuInitData();

                        params->iPad = m_iPad;

                        params->iSaveGameInfoIndex = -1;

                        params->levelGen = levelGen;

                        params->saveDetails = nullptr;

                        ui.NavigateToScene(m_iPad, eUIScene_LoadMenu, params);

                    }

                }

            }

        }

        break;



    case eControl_SavesList:

        {

            m_bIgnoreInput = true;

            ui.PlayUISFX(eSFX_Press);



            int saveIndex = static_cast<int>(childId);

            if(app.DebugSettingsOn() && app.GetLoadSavesFromFolderEnabled())

            {

                LoadSaveFromDisk(m_saves->at(saveIndex));

            }

            else

            {

                LoadMenuInitData *params = new LoadMenuInitData();

                params->iPad = m_iPad;

                params->iSaveGameInfoIndex = m_saveDetails[saveIndex].saveId;

                params->levelGen = nullptr;

                params->saveDetails = &m_saveDetails[saveIndex];

                ui.NavigateToScene(m_iPad, eUIScene_LoadMenu, params);

            }

        }

        break;



    case eControl_GamesList:

        {

#ifdef _WINDOWS64

            if (static_cast<int>(childId) == ADD_SERVER_BUTTON_INDEX)

            {

                ui.PlayUISFX(eSFX_Press);

                BeginAddServer();

                break;

            }

#endif

            m_bIgnoreInput = true;

            m_eAction = eAction_JoinGame;

            ui.PlayUISFX(eSFX_Press);



            int nIndex = static_cast<int>(childId);

#ifdef _WINDOWS64

            nIndex -= 1;

#endif

            m_iGameListIndex = nIndex;

            CheckAndJoinGame(nIndex);

        }

        break;

    }

}

void UIScene_LoadCreateJoinMenu::CheckAndJoinGame(int gameIndex)

{

	if( m_buttonListGames.getItemCount() > 0 && gameIndex < m_currentSessions->size() )

	{

#if defined(__PS3__) || defined(__ORBIS__) || defined(__PSVITA__)

		// 4J-PB - is the player allowed to join games?

		bool noUGC=false;

		bool bContentRestricted=false;



		// we're online, since we are joining a game

		ProfileManager.GetChatAndContentRestrictions(m_iPad,true,&noUGC,&bContentRestricted,nullptr);



#ifdef __ORBIS__

		// 4J Stu - On PS4 we don't restrict playing multiplayer based on chat restriction, so remove this check

		noUGC = false;



		bool bPlayStationPlus=true;

		int iPadWithNoPlaystationPlus=0;

		bool isSignedInLive = true;

		int iPadNotSignedInLive = -1;

		for(unsigned int i = 0; i < XUSER_MAX_COUNT; ++i)

		{

			if( InputManager.IsPadConnected(i) || ProfileManager.IsSignedIn(i) )

			{

				if (isSignedInLive && !ProfileManager.IsSignedInLive(i))

				{

					// Record the first non signed in live pad

					iPadNotSignedInLive = i;

				}



				isSignedInLive = isSignedInLive && ProfileManager.IsSignedInLive(i);

				if(ProfileManager.HasPlayStationPlus(i)==false)

				{

					bPlayStationPlus=false;

					break;

				}

			}

		}

#endif

#ifdef __PSVITA__

		if( CGameNetworkManager::usingAdhocMode() )

		{

			bContentRestricted = false;

			noUGC = false;

		}

#endif



		if(noUGC)

		{

			// not allowed to join

#ifndef __PSVITA__

			UINT uiIDA[1];

			uiIDA[0]=IDS_CONFIRM_OK;

			// Not allowed to play online

			ui.RequestAlertMessage(IDS_ONLINE_GAME, IDS_CHAT_RESTRICTION_UGC, uiIDA, 1, m_iPad,nullptr,this);

#else

			// Not allowed to play online

			ProfileManager.ShowSystemMessage( SCE_MSG_DIALOG_SYSMSG_TYPE_TRC_PSN_CHAT_RESTRICTION, 0 );

#endif



			m_bIgnoreInput=false;

			return;

		}

		else if(bContentRestricted)

		{

			ui.RequestContentRestrictedMessageBox();



			m_bIgnoreInput=false;

			return;

		}

#ifdef __ORBIS__

		// If this is an online game but not all players are signed in to Live, stop!

		else if (!isSignedInLive)

		{

			UINT uiIDA[1];

			uiIDA[0]=IDS_CONFIRM_OK;



			// Check if PSN is unavailable because of age restriction

			int npAvailability = ProfileManager.getNPAvailability(iPadNotSignedInLive);

			if (npAvailability == SCE_NP_ERROR_AGE_RESTRICTION)

			{

				m_bIgnoreInput = false;

				// 4J Stu - This is a bit messy and is due to the library incorrectly returning false for IsSignedInLive if the npAvailability isn't SCE_OK

				ui.RequestErrorMessage(IDS_ONLINE_SERVICE_TITLE, IDS_CONTENT_RESTRICTION, uiIDA, 1, iPadNotSignedInLive);

			}

			else

			{

				ui.RequestErrorMessage( IDS_PRO_NOTONLINE_TITLE, IDS_PRO_NOTONLINE_TEXT, uiIDA,1,iPadNotSignedInLive, &UIScene_LoadCreateJoinMenu::MustSignInReturnedPSN, this);

			}

			return;

		}

		else if(bPlayStationPlus==false)

		{



			if(ProfileManager.RequestingPlaystationPlus(iPadWithNoPlaystationPlus))

			{

				// MGH -  added this so we don't try and upsell when we don't know if the player has PS Plus yet (if it can't connect to the PS Plus server).

				UINT uiIDA[1];

				uiIDA[0]=IDS_OK;

				ui.RequestAlertMessage(IDS_ERROR_NETWORK_TITLE, IDS_ERROR_NETWORK, uiIDA, 1, ProfileManager.GetPrimaryPad(), nullptr, nullptr);

				return;

			}



			// PS Plus upsell

			// 4J-PB - we're not allowed to show the text Playstation Plus - have to call the upsell all the time!

			// upsell psplus

			int32_t iResult=sceNpCommerceDialogInitialize();



			SceNpCommerceDialogParam param;

			sceNpCommerceDialogParamInitialize(&param);

			param.mode=SCE_NP_COMMERCE_DIALOG_MODE_PLUS;

			param.features = SCE_NP_PLUS_FEATURE_REALTIME_MULTIPLAY;

			param.userId = ProfileManager.getUserID(iPadWithNoPlaystationPlus);



			iResult=sceNpCommerceDialogOpen(&param);



			//                     UINT uiIDA[2];

			//                     uiIDA[0]=IDS_CONFIRM_OK;

			//                     uiIDA[1]=IDS_PLAYSTATIONPLUS_SIGNUP;

			//                     ui.RequestMessageBox( IDS_FAILED_TO_CREATE_GAME_TITLE, IDS_NO_PLAYSTATIONPLUS, uiIDA,2,ProfileManager.GetPrimaryPad(),&UIScene_LoadCreateJoinMenu::PSPlusReturned,this, app.GetStringTable(),nullptr,0,false);



			m_bIgnoreInput=false;

			return;

		}



#endif

#endif



		m_initData->iPad = 0;;

		m_initData->selectedSession = m_currentSessions->at( gameIndex );

#ifdef _WINDOWS64

		{



			int serverDbCount = 0;

			FILE* dbFile = fopen("servers.db", "rb");

			if (dbFile)

			{

				char magic[4] = {};

				if (fread(magic, 1, 4, dbFile) == 4 && memcmp(magic, "MCSV", 4) == 0)

				{

					uint32_t version = 0, count = 0;

					fread(&version, sizeof(uint32_t), 1, dbFile);

					fread(&count, sizeof(uint32_t), 1, dbFile);

					if (version == 1)

						serverDbCount = static_cast<int>(count);

				}

				fclose(dbFile);

			}

			int lanCount = static_cast<int>(m_currentSessions->size()) - serverDbCount;

			if (gameIndex >= lanCount && lanCount >= 0)

				m_initData->serverIndex = gameIndex - lanCount;

			else

				m_initData->serverIndex = -1; 

		}

#endif



		if(m_initData->selectedSession->data.texturePackParentId!=0)

		{

			int texturePacksCount = Minecraft::GetInstance()->skins->getTexturePackCount();

			bool bHasTexturePackInstalled=false;



			for(int i=0;i<texturePacksCount;i++)

			{

				TexturePack *tp = Minecraft::GetInstance()->skins->getTexturePackByIndex(i);

				if(tp->getDLCParentPackId()==m_initData->selectedSession->data.texturePackParentId)

				{

					bHasTexturePackInstalled=true;

					break;

				}

			}



			if(bHasTexturePackInstalled==false)

			{



#ifdef _XBOX

				ULONGLONG ullOfferID_Full;

				app.GetDLCFullOfferIDForPackID(m_initData->selectedSession->data.texturePackParentId,&ullOfferID_Full);



				TelemetryManager->RecordUpsellPresented(m_iPad, eSet_UpsellID_Texture_DLC, ullOfferID_Full & 0xFFFFFFFF);

#endif

				UINT uiIDA[2];



				uiIDA[0]=IDS_TEXTUREPACK_FULLVERSION;

				//uiIDA[1]=IDS_TEXTURE_PACK_TRIALVERSION;

				uiIDA[1]=IDS_CONFIRM_CANCEL;



				ui.RequestAlertMessage(IDS_DLC_TEXTUREPACK_NOT_PRESENT_TITLE, IDS_DLC_TEXTUREPACK_NOT_PRESENT, uiIDA, 2, m_iPad,&UIScene_LoadCreateJoinMenu::TexturePackDialogReturned,this);



				return;

			}



#ifdef __PSVITA__

			if(CGameNetworkManager::usingAdhocMode() && !SQRNetworkManager_AdHoc_Vita::GetAdhocStatus())

			{

				// not connected to adhoc anymore, must have connected back to PSN to buy texture pack so sign in again

				SQRNetworkManager_AdHoc_Vita::AttemptAdhocSignIn(&UIScene_LoadCreateJoinMenu::SignInAdhocReturned, this);

				return;

		}

#endif

		}

		m_controlJoinTimer.setVisible( false );



#ifdef _XBOX

		XBackgroundDownloadSetMode(XBACKGROUND_DOWNLOAD_MODE_AUTO);

#endif



		m_bIgnoreInput=true;

		ui.NavigateToScene(ProfileManager.GetPrimaryPad(),eUIScene_JoinMenu,m_initData);

	}

}



void UIScene_LoadCreateJoinMenu::LoadLevelGen(LevelGenerationOptions *levelGen)

{
    // Load data from disc

    //File saveFile( L"Tutorial\\Tutorial" );

    //LoadSaveFromDisk(&saveFile);



    // clear out the app's terrain features list

    app.ClearTerrainFeaturePosition();



    StorageManager.ResetSaveData();

    // Make our next save default to the name of the level

    const wstring preferredSaveTitle = GetPreferredLevelGenSaveTitle(levelGen);
    StorageManager.SetSaveTitle(preferredSaveTitle.empty() ? levelGen->getDefaultSaveName().c_str() : preferredSaveTitle.c_str());



    bool isClientSide = false;

    bool isPrivate = false;

    // TODO int maxPlayers = MINECRAFT_NET_MAX_PLAYERS;

    int maxPlayers = 8;



    if( app.GetTutorialMode() )

    {

        isClientSide = false;

        maxPlayers = 4;

    }



    g_NetworkManager.HostGame(0,isClientSide,isPrivate,maxPlayers,0);



    NetworkGameInitData *param = new NetworkGameInitData();

    param->seed = 0;

    param->saveData = nullptr;

    param->settings = app.GetGameHostOption( eGameHostOption_Tutorial );

    param->levelGen = levelGen;
	param->levelName = preferredSaveTitle;



    if(levelGen->requiresTexturePack())

    {

        param->texturePackId = levelGen->getRequiredTexturePackId();



        Minecraft *pMinecraft = Minecraft::GetInstance();

        pMinecraft->skins->selectTexturePackById(param->texturePackId);

        //pMinecraft->skins->updateUI();

    }



#ifndef _XBOX

    g_NetworkManager.FakeLocalPlayerJoined();

#endif



    LoadingInputParams *loadingParams = new LoadingInputParams();

    loadingParams->func = &CGameNetworkManager::RunNetworkGameThreadProc;

    loadingParams->lpParam = static_cast<LPVOID>(param);



    UIFullscreenProgressCompletionData *completionData = new UIFullscreenProgressCompletionData();

    completionData->bShowBackground=TRUE;

    completionData->bShowLogo=TRUE;

    completionData->type = e_ProgressCompletion_CloseAllPlayersUIScenes;

    completionData->iPad = DEFAULT_XUI_MENU_USER;

    loadingParams->completionData = completionData;



    ui.NavigateToScene(ProfileManager.GetPrimaryPad(),eUIScene_FullscreenProgress, loadingParams);

}



void UIScene_LoadCreateJoinMenu::UpdateGamesListCallback(LPVOID pParam)

{

    if(pParam != nullptr)

    {

        UIScene_LoadCreateJoinMenu *pScene = static_cast<UIScene_LoadCreateJoinMenu *>(pParam);

        pScene->UpdateGamesList();

    }

}

void UIScene_LoadCreateJoinMenu::RebuildJoinGamesListVisual(bool syncFocus)

{
    if (m_bRebuildingJoinVisual)
    {
        return;
    }

    m_bRebuildingJoinVisual = true;

    FriendSessionInfo *pSelectedSession = nullptr;

    if(DoesGamesListHaveFocus() && m_buttonListGames.getItemCount() > 0)

    {

        unsigned int nIndex = m_buttonListGames.getCurrentSelection();

#ifdef _WINDOWS64

        if (m_currentSessions != nullptr && nIndex > 0 && (nIndex - 1) < m_currentSessions->size())

            pSelectedSession = m_currentSessions->at( nIndex - 1 );

#else

        if (m_currentSessions != nullptr && nIndex < m_currentSessions->size())

            pSelectedSession = m_currentSessions->at( nIndex );

#endif

    }

    SessionID selectedSessionId;

	ZeroMemory(&selectedSessionId,sizeof(SessionID));
    if( pSelectedSession != nullptr )selectedSessionId = pSelectedSession->sessionId;

    m_buttonListGames.clearList();

#ifdef _WINDOWS64

    m_buttonListGames.addItem(wstring(L"Add Server"));
    TrySetButtonListIcon(
        m_buttonListGames,
        m_buttonListGames.getItemCount() - 1,
        L"lceheadwer.png",
        L"lceheadwer");

#endif

    if (m_currentSessions == nullptr || m_currentSessions->empty())
    {
        m_buttonListGames.setCurrentSelection(0);
        if(syncFocus)
            m_buttonListGames.updateChildFocus(0);
        m_bRebuildingJoinVisual = false;
        return;
    }

    unsigned int sessionIndex = 0;

    m_buttonListGames.setCurrentSelection(0);

    for( FriendSessionInfo *sessionInfo : *m_currentSessions )

    {

        wchar_t textureName[64] = L"\0";

        if(sessionInfo->data.texturePackParentId!=0)

        {

            Minecraft *pMinecraft = Minecraft::GetInstance();
            TexturePack *tp = pMinecraft->skins->getTexturePackById(sessionInfo->data.texturePackParentId);
            DWORD dwImageBytes=0;
            PBYTE pbImageData=nullptr;

            if(tp==nullptr)

            {

                DWORD dwBytes=0;
                PBYTE pbData=nullptr;
                app.GetTPD(sessionInfo->data.texturePackParentId,&pbData,&dwBytes);
                app.GetFileFromTPD(eTPDFileType_Icon,pbData,dwBytes,&pbImageData,&dwImageBytes );

                if(dwImageBytes > 0 && pbImageData)

                {

                    swprintf(textureName,64,L"%ls",sessionInfo->displayLabel);
                    registerSubstitutionTexture(textureName,pbImageData,dwImageBytes);

                }

            }

            else

            {

                pbImageData = tp->getPackIcon(dwImageBytes);
                if(dwImageBytes > 0 && pbImageData)

                {

                    swprintf(textureName,64,L"%ls",sessionInfo->displayLabel);
                    registerSubstitutionTexture(textureName,pbImageData,dwImageBytes);

                }

            }

        }

        else

        {

            Minecraft *pMinecraft = Minecraft::GetInstance();
            TexturePack *tp = pMinecraft->skins->getTexturePackByIndex(0);
            DWORD dwImageBytes;
            PBYTE pbImageData = tp->getPackIcon(dwImageBytes);
            if(dwImageBytes > 0 && pbImageData)

            {

                swprintf(textureName,64,L"%ls",sessionInfo->displayLabel);
                registerSubstitutionTexture(textureName,pbImageData,dwImageBytes);

            }

        }

        m_buttonListGames.addItem( sessionInfo->displayLabel, textureName );

        if(memcmp( &selectedSessionId, &sessionInfo->sessionId, sizeof(SessionID) ) == 0)

        {

#ifdef _WINDOWS64

            m_buttonListGames.setCurrentSelection(sessionIndex + 1);

#else

            m_buttonListGames.setCurrentSelection(sessionIndex);

#endif

        }

        ++sessionIndex;

    }

    if(syncFocus)
        m_buttonListGames.updateChildFocus(m_buttonListGames.getCurrentSelection());

    m_bRebuildingJoinVisual = false;

}



void UIScene_LoadCreateJoinMenu::UpdateGamesList()

{
    // If we're ignoring input scene isn't active so do nothing

    if (m_bIgnoreInput) return;



    // If a texture pack is loading, or will be loading, then ignore this ( we are going to be destroyed anyway)

    if( Minecraft::GetInstance()->skins->getSelected()->isLoadingData() || (Minecraft::GetInstance()->skins->needsUIUpdate() || ui.IsReloadingSkin()) ) return;



    // if we're retrieving save info, don't show the list yet as we will be ignoring press events

    if(!m_bSavesDisplayed)

    {

        return;

    }


    m_controlJoinTimer.setVisible( false );



    // if the saves list has focus, then we should show the Delete Save tooltip

    // if the games list has focus, then we should show the View Gamercard tooltip

    int iRB=-1;

    int iY = -1;

    int iX=-1;



    vector<FriendSessionInfo*>* newSessions = g_NetworkManager.GetSessionList( m_iPad, 1, m_bShowingPartyGamesOnly );



    if (m_currentSessions != NULL && m_currentSessions->size() == newSessions->size())

    {

        bool same = true;

        for (size_t i = 0; i < newSessions->size(); i++)

        {

            if (memcmp(&(*m_currentSessions)[i]->sessionId, &(*newSessions)[i]->sessionId, sizeof(SessionID)) != 0 ||

                wcscmp((*m_currentSessions)[i]->displayLabel ? (*m_currentSessions)[i]->displayLabel : L"",

                       (*newSessions)[i]->displayLabel ? (*newSessions)[i]->displayLabel : L"") != 0)

            {

                same = false;

                break;

            }

        }

        if (same)

        {

            for (auto& it : *newSessions)

                delete it;

            delete newSessions;

            return;

        }

    }



    if (m_currentSessions)

    {

        for (auto& it : *m_currentSessions)

            delete it;

        delete m_currentSessions;

    }

    m_currentSessions = newSessions;

    unsigned int filteredListSize = static_cast<unsigned int>(m_currentSessions->size());



    BOOL gamesListHasFocus = DoesGamesListHaveFocus();



    if(filteredListSize > 0)

    {

#if TO_BE_IMPLEMENTED

        if( !m_pGamesList->IsEnabled() )

        {

            m_pGamesList->SetEnable(TRUE);

            m_pGamesList->SetCurSel( 0 );

        }

#endif

        m_bHasNoGamesLabel = false;

        m_controlJoinTimer.setVisible( false );

    }

    else

    {

#if TO_BE_IMPLEMENTED

        m_pGamesList->SetEnable(FALSE);

#endif

        m_controlJoinTimer.setVisible( false );

        if(m_bHasNoGamesLabel) m_labelNoGames.setVisible( true );



#if TO_BE_IMPLEMENTED

        if( gamesListHasFocus ) m_pGamesList->InitFocus(m_iPad);

#endif

    }

    if (m_activeTab == eTab_Join)
    {
        RebuildJoinGamesListVisual(true);
        m_bPendingJoinVisualRefresh = false;
    }
    else
    {
        m_bPendingJoinVisualRefresh = true;
    }



	updateTooltips();

    m_bPendingJoinTabAvailabilityRefresh = true;

}



void UIScene_LoadCreateJoinMenu::HandleDLCMountingComplete()

{

    Initialise();

}



bool UIScene_LoadCreateJoinMenu::DoesSavesListHaveFocus()

{

	return (m_activeTab == eTab_Load) && m_buttonListSaves.hasFocus();

}

bool UIScene_LoadCreateJoinMenu::DoesMashUpWorldHaveFocus()

{

	if(!m_buttonListNewGames.hasFocus())

	{

		return false;

	}



	const int lGenID = m_iNewGameListIndex - m_iDefaultButtonsC;

	if(lGenID < 0 || lGenID >= static_cast<int>(m_generators.size()))

	{

		return false;

	}

	LevelGenerationOptions *levelGen = m_generators.at(lGenID);

	return levelGen && !levelGen->isTutorial() && levelGen->requiresTexturePack();

}

bool UIScene_LoadCreateJoinMenu::DoesGamesListHaveFocus()

{

    return (m_activeTab == eTab_Join) && m_buttonListGames.hasFocus();

}



bool UIScene_LoadCreateJoinMenu::DoesNewGamesListHaveFocus()

{

    return (m_activeTab == eTab_Create) && m_buttonListNewGames.hasFocus();

}



void UIScene_LoadCreateJoinMenu::handleTimerComplete(int id)

{

    switch(id)

    {

    case JOIN_LOAD_ONLINE_TIMER_ID:

        {

#ifdef _XBOX

            XPARTY_USER_LIST partyList;



            if((XPartyGetUserList(  &partyList ) != XPARTY_E_NOT_IN_PARTY ) && (partyList.dwUserCount>1))

            {

                m_bInParty=true;

            }

            else

            {

                m_bInParty=false;

            }

#endif



            bool bMultiplayerAllowed = ProfileManager.IsSignedInLive( m_iPad ) && ProfileManager.AllowedToPlayMultiplayer(m_iPad);

            if(bMultiplayerAllowed != m_bMultiplayerAllowed)

            {

                if( bMultiplayerAllowed )

                {

                    // 					m_CheckboxOnline.SetEnable(TRUE);

                    // 					m_CheckboxPrivate.SetEnable(TRUE);

                }

                else

                {

                    m_bInParty = false;

                    m_buttonListGames.clearList();

                    m_controlJoinTimer.setVisible( false );

                    m_bHasNoGamesLabel = false;

                }



                m_bMultiplayerAllowed = bMultiplayerAllowed;

            }

        }

        break;

        // 4J-PB - Only Xbox will not have trial DLC patched into the game

#ifdef _XBOX

    case CHECKFORAVAILABLETEXTUREPACKS_TIMER_ID:

        {



#if defined(__PS3__) || defined(__ORBIS__) || defined(__PSVITA__)

            for(int i=0;i<m_iTexturePacksNotInstalled;i++)

            {

                if(m_iConfigA[i]!=-1)

                {

                    DLC_INFO *pDLCInfo=app.GetDLCInfoFromTPackID(m_iConfigA[i]);



                    if(pDLCInfo)

                    {

                        // retrieve the image - if we haven't already

                        wstring textureName = filenametowstring(pDLCInfo->chImageURL);



                        if(hasRegisteredSubstitutionTexture(textureName)==false)

                        {

                            PBYTE pbImageData;

                            int iImageDataBytes=0;

                            SonyHttp::getDataFromURL(pDLCInfo->chImageURL,(void **)&pbImageData,&iImageDataBytes);



                            if(iImageDataBytes!=0)

                            {

                                // set the image

                                registerSubstitutionTexture(textureName,pbImageData,iImageDataBytes,true);

                                m_iConfigA[i]=-1;

                            }



                        }

                    }

                }

            }



            bool bAllDone=true;

            for(int i=0;i<m_iTexturePacksNotInstalled;i++)

            {

                if(m_iConfigA[i]!=-1)

                {

                    bAllDone = false;

                }

            }



            if(bAllDone)

            {

                // kill this timer

                killTimer(CHECKFORAVAILABLETEXTUREPACKS_TIMER_ID);

            }

#endif



        }

        break;

#endif

    }



}



void UIScene_LoadCreateJoinMenu::LoadSaveFromDisk(File *saveFile, ESavePlatform savePlatform /*= SAVE_FILE_PLATFORM_LOCAL*/)

{

    // we'll only be coming in here when the tutorial is loaded now



    StorageManager.ResetSaveData();



    // Make our next save default to the name of the level

    StorageManager.SetSaveTitle(saveFile->getName().c_str());



    int64_t fileSize = saveFile->length();

    FileInputStream fis(*saveFile);

    byteArray ba(static_cast<unsigned int>(fileSize));

    fis.read(ba);

    fis.close();







    bool isClientSide = false;

    bool isPrivate = false;

    int maxPlayers = MINECRAFT_NET_MAX_PLAYERS;



    if( app.GetTutorialMode() )

    {

        isClientSide = false;

        maxPlayers = 4;

    }



    app.SetGameHostOption(eGameHostOption_GameType,GameType::CREATIVE->getId() );



    g_NetworkManager.HostGame(0,isClientSide,isPrivate,maxPlayers,0);



    LoadSaveDataThreadParam *saveData = new LoadSaveDataThreadParam(ba.data, ba.length, saveFile->getName());



    NetworkGameInitData *param = new NetworkGameInitData();

    param->seed = 0;

    param->saveData = saveData;

    param->settings = app.GetGameHostOption( eGameHostOption_All );

    param->savePlatform = savePlatform;



#ifndef _XBOX

    g_NetworkManager.FakeLocalPlayerJoined();

#endif



    LoadingInputParams *loadingParams = new LoadingInputParams();

    loadingParams->func = &CGameNetworkManager::RunNetworkGameThreadProc;

    loadingParams->lpParam = static_cast<LPVOID>(param);



    UIFullscreenProgressCompletionData *completionData = new UIFullscreenProgressCompletionData();

    completionData->bShowBackground=TRUE;

    completionData->bShowLogo=TRUE;

    completionData->type = e_ProgressCompletion_CloseAllPlayersUIScenes;

    completionData->iPad = DEFAULT_XUI_MENU_USER;

    loadingParams->completionData = completionData;



    ui.NavigateToScene(ProfileManager.GetPrimaryPad(),eUIScene_FullscreenProgress, loadingParams);

}



#ifdef SONY_REMOTE_STORAGE_DOWNLOAD

void UIScene_LoadCreateJoinMenu::LoadSaveFromCloud()

{



    wchar_t wFileName[128];

    mbstowcs(wFileName, app.getRemoteStorage()->getLocalFilename(), strlen(app.getRemoteStorage()->getLocalFilename())+1); // plus null

    File cloudFile(wFileName);





    StorageManager.ResetSaveData();



    // Make our next save default to the name of the level

    wchar_t wSaveName[128];

    mbstowcs(wSaveName, app.getRemoteStorage()->getSaveNameUTF8(), strlen(app.getRemoteStorage()->getSaveNameUTF8())+1); // plus null

    StorageManager.SetSaveTitle(wSaveName);



    int64_t fileSize = cloudFile.length();

    FileInputStream fis(cloudFile);

    byteArray ba(fileSize);

    fis.read(ba);

    fis.close();







    bool isClientSide = false;

    bool isPrivate = false;

    int maxPlayers = MINECRAFT_NET_MAX_PLAYERS;



    if( app.GetTutorialMode() )

    {

        isClientSide = false;

        maxPlayers = 4;

    }



	app.SetGameHostOption(eGameHostOption_All, app.getRemoteStorage()->getSaveHostOptions() );



    g_NetworkManager.HostGame(0,isClientSide,isPrivate,maxPlayers,0);



    LoadSaveDataThreadParam *saveData = new LoadSaveDataThreadParam(ba.data, ba.length, cloudFile.getName());



    NetworkGameInitData *param = new NetworkGameInitData();

    param->seed = app.getRemoteStorage()->getSaveSeed();

    param->saveData = saveData;

    param->settings = app.GetGameHostOption( eGameHostOption_All );

    param->savePlatform = app.getRemoteStorage()->getSavePlatform();

	param->texturePackId = app.getRemoteStorage()->getSaveTexturePack();



#ifndef _XBOX

    g_NetworkManager.FakeLocalPlayerJoined();

#endif



    LoadingInputParams *loadingParams = new LoadingInputParams();

    loadingParams->func = &CGameNetworkManager::RunNetworkGameThreadProc;

    loadingParams->lpParam = (LPVOID)param;



    UIFullscreenProgressCompletionData *completionData = new UIFullscreenProgressCompletionData();

    completionData->bShowBackground=TRUE;

    completionData->bShowLogo=TRUE;

    completionData->type = e_ProgressCompletion_CloseAllPlayersUIScenes;

    completionData->iPad = DEFAULT_XUI_MENU_USER;

    loadingParams->completionData = completionData;



    ui.NavigateToScene(ProfileManager.GetPrimaryPad(),eUIScene_FullscreenProgress, loadingParams);

}



#endif //SONY_REMOTE_STORAGE_DOWNLOAD



#ifdef _WINDOWS64

static bool Win64_DeleteSaveDirectory(const wchar_t* wPath)

{

    wchar_t wSearch[MAX_PATH];

    swprintf_s(wSearch, MAX_PATH, L"%s\\*", wPath);

    WIN32_FIND_DATAW fd;

    HANDLE hFind = FindFirstFileW(wSearch, &fd);

    if (hFind != INVALID_HANDLE_VALUE)

    {

        do

        {

            if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;

            wchar_t wChild[MAX_PATH];

            swprintf_s(wChild, MAX_PATH, L"%s\\%s", wPath, fd.cFileName);

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)

                Win64_DeleteSaveDirectory(wChild);

            else

                DeleteFileW(wChild);

        } while (FindNextFileW(hFind, &fd));

        FindClose(hFind);

    }

    return RemoveDirectoryW(wPath) != 0;

}

#endif // _WINDOWS64



int UIScene_LoadCreateJoinMenu::DeleteSaveDialogReturned(void *pParam,int iPad,C4JStorage::EMessageResult result)

{

    UIScene_LoadCreateJoinMenu* pClass = static_cast<UIScene_LoadCreateJoinMenu *>(pParam);

    // results switched for this dialog



	// Check that we have a valid save selected (can get a bad index if the save list has been refreshed)

	bool validSelection= pClass->m_iSaveListIndex >= 0 && pClass->m_iSaveListIndex < pClass->m_buttonListSaves.getItemCount();



    if(result==C4JStorage::EMessage_ResultDecline && validSelection)

    {

        if(app.DebugSettingsOn() && app.GetLoadSavesFromFolderEnabled())

        {

            pClass->m_bIgnoreInput=false;

        }

        else

        {

#ifdef _WINDOWS64

            {

                // Use m_saveDetails (sorted display order) so the correct folder is targeted

                int displayIdx = pClass->m_iSaveListIndex;

                bool bSuccess = false;

                if (pClass->m_saveDetails && displayIdx >= 0 && pClass->m_saveDetails[displayIdx].UTF8SaveFilename[0])

                {

                    wchar_t wFilename[MAX_SAVEFILENAME_LENGTH] = {};

                    mbstowcs_s(nullptr, wFilename, MAX_SAVEFILENAME_LENGTH, pClass->m_saveDetails[displayIdx].UTF8SaveFilename, MAX_SAVEFILENAME_LENGTH - 1);

                    wchar_t wFolderPath[MAX_PATH] = {};

                    swprintf_s(wFolderPath, MAX_PATH, L"Windows64\\GameHDD\\%s", wFilename);

                    bSuccess = Win64_DeleteSaveDirectory(wFolderPath);

                }

                UIScene_LoadCreateJoinMenu::DeleteSaveDataReturned((LPVOID)pClass->GetCallbackUniqueId(), bSuccess);

            }

#else

			StorageManager.DeleteSaveData(&pClass->m_pSaveDetails->SaveInfoA[pClass->m_iSaveListIndex], UIScene_LoadCreateJoinMenu::DeleteSaveDataReturned, (LPVOID)pClass->GetCallbackUniqueId());

#endif

            pClass->m_controlSavesTimer.setVisible( true );

        }

    }

    else

    {

        pClass->m_bIgnoreInput=false;

    }



    return 0;

}



int UIScene_LoadCreateJoinMenu::DeleteSaveDataReturned(LPVOID lpParam,bool bRes)

{

	ui.EnterCallbackIdCriticalSection();

    UIScene_LoadCreateJoinMenu* pClass = static_cast<UIScene_LoadCreateJoinMenu *>(ui.GetSceneFromCallbackId((size_t)lpParam));



	if(pClass)

	{

		if(bRes)

		{

			// wipe the list and repopulate it

			pClass->m_iState=e_SavesRepopulateAfterDelete;

		}

		else pClass->m_bIgnoreInput=false;



		pClass->updateTooltips();

	}

	ui.LeaveCallbackIdCriticalSection();

    return 0;

}





int UIScene_LoadCreateJoinMenu::RenameSaveDataReturned(LPVOID lpParam,bool bRes)

{

    UIScene_LoadCreateJoinMenu* pClass = static_cast<UIScene_LoadCreateJoinMenu *>(lpParam);



    if(bRes)

    {

        pClass->m_iState=e_SavesRepopulate;

    }

    else pClass->m_bIgnoreInput=false;



    pClass->updateTooltips();



    return 0;

}



#ifdef __ORBIS__





void UIScene_LoadCreateJoinMenu::LoadRemoteFileFromDisk(char* remoteFilename)

{

    wchar_t wSaveName[128];

    mbstowcs(wSaveName, remoteFilename, strlen(remoteFilename)+1); // plus null



    // 	processConsoleSave(wSaveName, L"ProcessedSave.bin");



    // 	File remoteFile(L"ProcessedSave.bin");

    File remoteFile(wSaveName);

    LoadSaveFromDisk(&remoteFile, SAVE_FILE_PLATFORM_PS3);

}

#endif





int UIScene_LoadCreateJoinMenu::SaveOptionsDialogReturned(void *pParam,int iPad,C4JStorage::EMessageResult result)

{

    UIScene_LoadCreateJoinMenu* pClass = static_cast<UIScene_LoadCreateJoinMenu *>(pParam);



    // results switched for this dialog

    // EMessage_ResultAccept means cancel

    switch(result)

    {

    case C4JStorage::EMessage_ResultDecline:  // rename

        {

			pClass->m_bIgnoreInput=true;

#ifdef _WINDOWS64

            {

                wchar_t wSaveName[128];

                ZeroMemory(wSaveName, 128 * sizeof(wchar_t));

                mbstowcs_s(nullptr, wSaveName, 128, pClass->m_saveDetails[pClass->m_iSaveListIndex].UTF8SaveName, _TRUNCATE);

                UIKeyboardInitData kbData;

                kbData.title       = app.GetString(IDS_RENAME_WORLD_TITLE);

                kbData.defaultText = wSaveName;

                kbData.maxChars    = 25;

                kbData.callback    = &UIScene_LoadCreateJoinMenu::KeyboardCompleteWorldNameCallback;

                kbData.lpParam     = pClass;

                kbData.pcMode      = g_KBMInput.IsKBMActive();

                ui.NavigateToScene(pClass->m_iPad, eUIScene_Keyboard, &kbData);

            }

#elif defined _DURANGO

            // bring up a keyboard

            InputManager.RequestKeyboard(app.GetString(IDS_RENAME_WORLD_TITLE), (pClass->m_saveDetails[pClass->m_iSaveListIndex]).UTF16SaveName,(DWORD)0,25,&UIScene_LoadCreateJoinMenu::KeyboardCompleteWorldNameCallback,pClass,C_4JInput::EKeyboardMode_Default);

#else

            // bring up a keyboard

            wchar_t wSaveName[128];

            //CD - Fix - We must memset the SaveName

            ZeroMemory(wSaveName, 128 * sizeof(wchar_t) );

            mbstowcs(wSaveName, pClass->m_saveDetails[pClass->m_iSaveListIndex].UTF8SaveName, strlen(pClass->m_saveDetails->UTF8SaveName)+1); // plus null

            LPWSTR ptr = wSaveName;

            InputManager.RequestKeyboard(app.GetString(IDS_RENAME_WORLD_TITLE),wSaveName,(DWORD)0,25,&UIScene_LoadCreateJoinMenu::KeyboardCompleteWorldNameCallback,pClass,C_4JInput::EKeyboardMode_Default);

#endif

        }

        break;



    case C4JStorage::EMessage_ResultThirdOption:  // delete -

        {

            // delete the save game

            // Have to ask the player if they are sure they want to delete this game

            UINT uiIDA[2];

            uiIDA[0]=IDS_CONFIRM_CANCEL;

            uiIDA[1]=IDS_CONFIRM_OK;

            ui.RequestAlertMessage(IDS_TOOLTIPS_DELETESAVE, IDS_TEXT_DELETE_SAVE, uiIDA, 2, iPad,&UIScene_LoadCreateJoinMenu::DeleteSaveDialogReturned,pClass);

        }

        break;



#ifdef SONY_REMOTE_STORAGE_UPLOAD

    case C4JStorage::EMessage_ResultFourthOption: // upload to cloud

        {

			UINT uiIDA[2];

			uiIDA[0]=IDS_CONFIRM_OK;

			uiIDA[1]=IDS_CONFIRM_CANCEL;



			ui.RequestAlertMessage(IDS_TOOLTIPS_SAVETRANSFER_UPLOAD, IDS_SAVE_TRANSFER_TEXT, uiIDA, 2, iPad,&UIScene_LoadCreateJoinMenu::SaveTransferDialogReturned,pClass);

        }

        break;

#endif // SONY_REMOTE_STORAGE_UPLOAD

#if defined _XBOX_ONE  || defined __ORBIS__

    case C4JStorage::EMessage_ResultFourthOption: // copy save

        {

			UINT uiIDA[2];

			uiIDA[0]=IDS_CONFIRM_OK;

			uiIDA[1]=IDS_CONFIRM_CANCEL;



			ui.RequestAlertMessage(IDS_COPYSAVE, IDS_TEXT_COPY_SAVE, uiIDA, 2, iPad,&UIScene_LoadCreateJoinMenu::CopySaveDialogReturned,pClass);

        }

        break;

#endif



    case C4JStorage::EMessage_Cancelled:

    default:

        {

            // reset the tooltips

            pClass->updateTooltips();

            pClass->m_bIgnoreInput=false;

        }

        break;

    }

    return 0;

}





#if defined (__PSVITA__)



int UIScene_LoadCreateJoinMenu::SignInAdhocReturned(void *pParam,bool bContinue, int iPad)

{

	UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu*)pParam;

	pClass->m_bIgnoreInput = false;

	return 0;



}







int UIScene_LoadCreateJoinMenu::MustSignInTexturePack(void *pParam,int iPad,C4JStorage::EMessageResult result)

{

	UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu*)pParam;



	if(result==C4JStorage::EMessage_ResultAccept)

	{

		SQRNetworkManager_Vita::AttemptPSNSignIn(&UIScene_LoadCreateJoinMenu::MustSignInReturnedTexturePack, pClass);

	}

	else

	{

		pClass->m_bIgnoreInput = false;

	}



	return 0;

}





int UIScene_LoadCreateJoinMenu::MustSignInReturnedTexturePack(void *pParam,bool bContinue, int iPad)

{

	UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu*)pParam;



	int commerceState = app.GetCommerceState();

	while(	commerceState != CConsoleMinecraftApp::eCommerce_State_Offline &&

			commerceState != CConsoleMinecraftApp::eCommerce_State_Online &&

			commerceState != CConsoleMinecraftApp::eCommerce_State_Error)

	{

		Sleep(10);

		commerceState = app.GetCommerceState();

	}



	if(bContinue==true)

	{

		SONYDLC *pSONYDLCInfo=app.GetSONYDLCInfo(pClass->m_initData->selectedSession->data.texturePackParentId);

		if(pSONYDLCInfo!=nullptr)

		{

			char chName[42];

			char chKeyName[20];

			char chSkuID[SCE_NP_COMMERCE2_SKU_ID_LEN];



			memset(chSkuID,0,SCE_NP_COMMERCE2_SKU_ID_LEN);

			// we have to retrieve the skuid from the store info, it can't be hardcoded since Sony may change it.

			// So we assume the first sku for the product is the one we want

			// MGH -  keyname in the DLC file is 16 chars long, but there's no space for a nullptr terminating char

			memset(chKeyName, 0, sizeof(chKeyName));

			strncpy(chKeyName, pSONYDLCInfo->chDLCKeyname, 16);



#ifdef __ORBIS__

			strcpy(chName, chKeyName);

#else

			sprintf(chName,"%s-%s",app.GetCommerceCategory(),chKeyName);

#endif

			app.GetDLCSkuIDFromProductList(chName,chSkuID);

			// 4J-PB - need to check for an empty store

			if(app.CheckForEmptyStore(iPad)==false)

			{

				if(app.DLCAlreadyPurchased(chSkuID))

				{

					app.DownloadAlreadyPurchased(chSkuID);

				}

				else

				{

					app.Checkout(chSkuID);

				}

			}

		}

	}

	pClass->m_bIgnoreInput = false;

    return 0;

}



#endif



int UIScene_LoadCreateJoinMenu::TexturePackDialogReturned(void *pParam,int iPad,C4JStorage::EMessageResult result)

{

    UIScene_LoadCreateJoinMenu *pClass = static_cast<UIScene_LoadCreateJoinMenu *>(pParam);



    // Exit with or without saving

    if(result==C4JStorage::EMessage_ResultAccept)

    {

        // we need to enable background downloading for the DLC

        XBackgroundDownloadSetMode(XBACKGROUND_DOWNLOAD_MODE_ALWAYS_ALLOW);

#if defined __PSVITA__ || defined __PS3__ || defined __ORBIS__



#ifdef __PSVITA__

		if(!ProfileManager.IsSignedInLive(ProfileManager.GetPrimaryPad()) && CGameNetworkManager::usingAdhocMode())

		{

			// get them to sign in to online

			UINT uiIDA[2];

			uiIDA[0]=IDS_PRO_NOTONLINE_ACCEPT;

			uiIDA[1]=IDS_PRO_NOTONLINE_DECLINE;

			ui.RequestAlertMessage(IDS_PRO_NOTONLINE_TITLE, IDS_PRO_XBOXLIVE_NOTIFICATION, uiIDA, 2, ProfileManager.GetPrimaryPad(),&UIScene_LoadCreateJoinMenu::MustSignInTexturePack,pClass);

			return;

		}

#endif



		SONYDLC *pSONYDLCInfo=app.GetSONYDLCInfo(pClass->m_initData->selectedSession->data.texturePackParentId);

		if(pSONYDLCInfo!=nullptr)

		{

			char chName[42];

			char chKeyName[20];

			char chSkuID[SCE_NP_COMMERCE2_SKU_ID_LEN];



			memset(chSkuID,0,SCE_NP_COMMERCE2_SKU_ID_LEN);

			// we have to retrieve the skuid from the store info, it can't be hardcoded since Sony may change it.

			// So we assume the first sku for the product is the one we want

			// MGH -  keyname in the DLC file is 16 chars long, but there's no space for a nullptr terminating char

			memset(chKeyName, 0, sizeof(chKeyName));

			strncpy(chKeyName, pSONYDLCInfo->chDLCKeyname, 16);



#ifdef __ORBIS__

			strcpy(chName, chKeyName);

#else

			sprintf(chName,"%s-%s",app.GetCommerceCategory(),chKeyName);

#endif

			app.GetDLCSkuIDFromProductList(chName,chSkuID);

			// 4J-PB - need to check for an empty store

			if(app.CheckForEmptyStore(iPad)==false)

			{

				if(app.DLCAlreadyPurchased(chSkuID))

				{

					app.DownloadAlreadyPurchased(chSkuID);

				}

				else

				{

					app.Checkout(chSkuID);

				}

			}

		}

#endif





#if defined _XBOX_ONE

		if(ProfileManager.IsSignedIn(iPad))

		{

			if (ProfileManager.IsSignedInLive(iPad))

			{

				wstring ProductId;

				app.GetDLCFullOfferIDForPackID(pClass->m_initData->selectedSession->data.texturePackParentId,ProductId);



				StorageManager.InstallOffer(1,(WCHAR *)ProductId.c_str(),nullptr,nullptr);

			}

			else

			{

				// 4J-JEV: Fix for XB1: #165863 - XR-074: Compliance: With no active network connection user is unable to convert from Trial to Full texture pack and is not messaged why.

				UINT uiIDA[1] = { IDS_CONFIRM_OK };

				ui.RequestErrorMessage(IDS_PRO_NOTONLINE_TITLE, IDS_PRO_XBOXLIVE_NOTIFICATION, uiIDA, 1, iPad);

			}

		}

#endif



    }

    pClass->m_bIgnoreInput=false;

    return 0;

}



#if defined __PS3__ || defined __PSVITA__ || defined __ORBIS__

int UIScene_LoadCreateJoinMenu::MustSignInReturnedPSN(void *pParam,int iPad,C4JStorage::EMessageResult result)

{

    UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu*)pParam;



	if(result==C4JStorage::EMessage_ResultAccept)

	{

#if defined(__PS3__)

		SQRNetworkManager_PS3::AttemptPSNSignIn(&UIScene_LoadCreateJoinMenu::PSN_SignInReturned, pClass);

#elif defined __PSVITA__

		SQRNetworkManager_Vita::AttemptPSNSignIn(&UIScene_LoadCreateJoinMenu::PSN_SignInReturned, pClass);

#else

		SQRNetworkManager_Orbis::AttemptPSNSignIn(&UIScene_LoadCreateJoinMenu::PSN_SignInReturned, pClass, false, iPad);

#endif

	}

	else

	{

		pClass->m_bIgnoreInput = false;

	}



    return 0;

}



int UIScene_LoadCreateJoinMenu::PSN_SignInReturned(void *pParam,bool bContinue, int iPad)

{

    UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu*)pParam;

	if(bContinue==true)

	{

		switch(pClass->m_eAction)

		{

		case eAction_ViewInvites:

			// Check if we're signed in to LIVE

			if(ProfileManager.IsSignedInLive(iPad))

			{

#if defined(__PS3__)

				int ret = sceNpBasicRecvMessageCustom(SCE_NP_BASIC_MESSAGE_MAIN_TYPE_INVITE, SCE_NP_BASIC_RECV_MESSAGE_OPTIONS_INCLUDE_BOOTABLE, SYS_MEMORY_CONTAINER_ID_INVALID);

				app.DebugPrintf("sceNpBasicRecvMessageCustom return %d ( %08x )\n", ret, ret);

#elif defined __PSVITA__

				SQRNetworkManager_Vita::RecvInviteGUI();

#else

				SQRNetworkManager_Orbis::RecvInviteGUI();

#endif

			}

			break;

		case eAction_JoinGame:

			pClass->CheckAndJoinGame(pClass->m_iGameListIndex);

			break;

		}

	}

	else

	{

		pClass->m_bIgnoreInput = false;

	}

    return 0;

}

#endif



#ifdef SONY_REMOTE_STORAGE_DOWNLOAD



void UIScene_LoadCreateJoinMenu::LaunchSaveTransfer()

{

    LoadingInputParams *loadingParams = new LoadingInputParams();

    loadingParams->func = &UIScene_LoadCreateJoinMenu::DownloadSonyCrossSaveThreadProc;

    loadingParams->lpParam = (LPVOID)this;



    UIFullscreenProgressCompletionData *completionData = new UIFullscreenProgressCompletionData();

    completionData->bShowBackground=TRUE;

    completionData->bShowLogo=TRUE;

    completionData->type = e_ProgressCompletion_NavigateBackToScene;

    completionData->iPad = DEFAULT_XUI_MENU_USER;

    loadingParams->completionData = completionData;



    loadingParams->cancelFunc=&UIScene_LoadCreateJoinMenu::CancelSaveTransferCallback;

	loadingParams->m_cancelFuncParam=this;

    loadingParams->cancelText=IDS_TOOLTIPS_CANCEL;



    ui.NavigateToScene(m_iPad,eUIScene_FullscreenProgress, loadingParams);

}









int UIScene_LoadCreateJoinMenu::CreateDummySaveDataCallback(LPVOID lpParam,bool bRes)

{

	UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu *) lpParam;

	if(bRes)

	{

		pClass->m_eSaveTransferState = eSaveTransfer_GetSavesInfo;

	}

	else

	{

		pClass->m_eSaveTransferState = eSaveTransfer_Error;

		app.DebugPrintf("CreateDummySaveDataCallback failed\n");



	}

	return 0;

}



int UIScene_LoadCreateJoinMenu::CrossSaveGetSavesInfoCallback(LPVOID lpParam, SAVE_DETAILS *pSaveDetails, bool bRes)

{

	UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu *) lpParam;

	if(bRes)

	{

		pClass->m_eSaveTransferState = eSaveTransfer_GetFileData;

	}

	else

	{

		pClass->m_eSaveTransferState = eSaveTransfer_Error;

		app.DebugPrintf("CrossSaveGetSavesInfoCallback failed\n");

	}

	return 0;

}



int UIScene_LoadCreateJoinMenu::LoadCrossSaveDataCallback( void *pParam,bool bIsCorrupt, bool bIsOwner )

{

	UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu *) pParam;

	if(bIsCorrupt == false && bIsOwner)

	{

		pClass->m_eSaveTransferState = eSaveTransfer_CreatingNewSave;

	}

	else

	{

		pClass->m_eSaveTransferState = eSaveTransfer_Error;

		app.DebugPrintf("LoadCrossSaveDataCallback failed \n");



	}

	return 0;

}



int UIScene_LoadCreateJoinMenu::CrossSaveFinishedCallback(void *pParam,int iPad,C4JStorage::EMessageResult result)

{

	UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu *) pParam;

	pClass->m_eSaveTransferState = eSaveTransfer_Idle;

	return 0;

}





int UIScene_LoadCreateJoinMenu::CrossSaveDeleteOnErrorReturned(LPVOID lpParam,bool bRes)

{

	UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu *) lpParam;

	pClass->m_eSaveTransferState = eSaveTransfer_ErrorMesssage;

	return 0;

}



int UIScene_LoadCreateJoinMenu::RemoteSaveNotFoundCallback(void *pParam,int iPad,C4JStorage::EMessageResult result)

{

	UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu *) pParam;

	pClass->m_eSaveTransferState = eSaveTransfer_Idle;

	return 0;

}



// MGH -  added this global to force the delete of the previous data, for the remote storage saves

//	need to speak to Chris why this is necessary

bool g_bForceVitaSaveWipe = false;





int UIScene_LoadCreateJoinMenu::DownloadSonyCrossSaveThreadProc( LPVOID lpParameter )

{

	m_bSaveTransferRunning = true;

#ifdef __PS3__

	StorageManager.SetSaveTransferInProgress(true);

#endif

    Compression::UseDefaultThreadStorage();

    UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu *) lpParameter;

 	pClass->m_saveTransferDownloadCancelled = false;

	m_bSaveTransferRunning = true;

	bool bAbortCalled = false;

	Minecraft *pMinecraft=Minecraft::GetInstance();

	bool bSaveFileCreated = false;

	wchar_t wSaveName[128];



    // get the save file size

	pMinecraft->progressRenderer->progressStagePercentage(0);

    pMinecraft->progressRenderer->progressStart(IDS_TOOLTIPS_SAVETRANSFER_DOWNLOAD);

    pMinecraft->progressRenderer->progressStage( IDS_TOOLTIPS_SAVETRANSFER_DOWNLOAD );



	ConsoleSaveFile* pSave = nullptr;



	pClass->m_eSaveTransferState = eSaveTransfer_GetRemoteSaveInfo;





    while(pClass->m_eSaveTransferState!=eSaveTransfer_Idle)

    {

        switch(pClass->m_eSaveTransferState)

        {

        case eSaveTransfer_Idle:

            break;

		case eSaveTransfer_GetRemoteSaveInfo:

			app.DebugPrintf("UIScene_LoadCreateJoinMenu getSaveInfo\n");

			app.getRemoteStorage()->getSaveInfo();

			pClass->m_eSaveTransferState = eSaveTransfer_GettingRemoteSaveInfo;

		break;

		case eSaveTransfer_GettingRemoteSaveInfo:

			if(pClass->m_saveTransferDownloadCancelled)

			{

				pClass->m_eSaveTransferState = eSaveTransfer_Error;

				break;

			}

			if(app.getRemoteStorage()->waitingForSaveInfo() == false)

			{

				if(app.getRemoteStorage()->saveIsAvailable())

				{

					if(app.getRemoteStorage()->saveVersionSupported())

					{

						pClass->m_eSaveTransferState = eSaveTransfer_CreateDummyFile;

					}

					else

					{

						// must be a newer version of the save in the cloud that we don't support yet

						UINT uiIDA[1];

						uiIDA[0]=IDS_CONFIRM_OK;

						ui.RequestAlertMessage(IDS_TOOLTIPS_SAVETRANSFER_DOWNLOAD, IDS_SAVE_TRANSFER_WRONG_VERSION, uiIDA, 1, ProfileManager.GetPrimaryPad(),RemoteSaveNotFoundCallback,pClass);

					}

				}

				else

				{

					// no save available, inform the user about the functionality

					UINT uiIDA[1];

					uiIDA[0]=IDS_CONFIRM_OK;

					ui.RequestAlertMessage(IDS_TOOLTIPS_SAVETRANSFER_DOWNLOAD, IDS_SAVE_TRANSFER_NOT_AVAILABLE_TEXT, uiIDA, 1, ProfileManager.GetPrimaryPad(),RemoteSaveNotFoundCallback,pClass);

				}

			}

			break;

		case eSaveTransfer_CreateDummyFile:

			{

				StorageManager.ResetSaveData();

				byte *compData = (byte *)StorageManager.AllocateSaveData( app.getRemoteStorage()->getSaveFilesize() );

				// Make our next save default to the name of the level

				const char* pNameUTF8 = app.getRemoteStorage()->getSaveNameUTF8();

				mbstowcs(wSaveName, pNameUTF8, strlen(pNameUTF8)+1); // plus null

				StorageManager.SetSaveTitle(wSaveName);

				PBYTE pbThumbnailData=nullptr;

				DWORD dwThumbnailDataSize=0;



				PBYTE pbDataSaveImage=nullptr;

				DWORD dwDataSizeSaveImage=0;



				StorageManager.GetDefaultSaveImage(&pbDataSaveImage, &dwDataSizeSaveImage);

				if(app.getRemoteStorage()->getThumbnailData() != nullptr && app.getRemoteStorage()->getThumbnailDataSize() > 0)
				{
					dwThumbnailDataSize = app.getRemoteStorage()->getThumbnailDataSize();
					pbThumbnailData = new BYTE[dwThumbnailDataSize];
					memcpy(pbThumbnailData, app.getRemoteStorage()->getThumbnailData(), dwThumbnailDataSize);
				}
				else
				{
					StorageManager.GetDefaultSaveThumbnail(&pbThumbnailData,&dwThumbnailDataSize);
				}



				BYTE bTextMetadata[88];

				ZeroMemory(bTextMetadata,88);

				unsigned int hostOptions = app.getRemoteStorage()->getSaveHostOptions();

#ifdef __ORBIS__

				app.SetGameHostOption(hostOptions, eGameHostOption_WorldSize, e_worldSize_Classic);		// force the classic world size on, otherwise it's unknown and we can't expand

#endif

				int iTextMetadataBytes = app.CreateImageTextData(bTextMetadata, app.getRemoteStorage()->getSaveSeed(), true, hostOptions, app.getRemoteStorage()->getSaveTexturePack() );



				// set the icon and save image

				StorageManager.SetSaveImages(pbThumbnailData,dwThumbnailDataSize,pbDataSaveImage,dwDataSizeSaveImage,bTextMetadata,iTextMetadataBytes);



				app.getRemoteStorage()->waitForStorageManagerIdle();

				C4JStorage::ESaveGameState saveState = StorageManager.SaveSaveData( &UIScene_LoadCreateJoinMenu::CreateDummySaveDataCallback, lpParameter );

				if(saveState == C4JStorage::ESaveGame_Save)

				{

					pClass->m_eSaveTransferState = eSaveTransfer_CreatingDummyFile;

				}

				else

				{

					app.DebugPrintf("Failed to create dummy save file\n");

					pClass->m_eSaveTransferState = eSaveTransfer_Error;

				}

			}

			break;

		case eSaveTransfer_CreatingDummyFile:

			break;

		case eSaveTransfer_GetSavesInfo:

			{

				// we can't cancel here, we need the saves info so we can delete the file

				if(pClass->m_saveTransferDownloadCancelled)

				{

					WCHAR wcTemp[256];

					swprintf(wcTemp,256, app.GetString(IDS_CANCEL));		// MGH - should change this string to "cancelling download"

					m_wstrStageText=wcTemp;

					pMinecraft->progressRenderer->progressStage( m_wstrStageText );

				}



				app.getRemoteStorage()->waitForStorageManagerIdle();

				app.DebugPrintf("CALL GetSavesInfo B\n");

				C4JStorage::ESaveGameState eSGIStatus= StorageManager.GetSavesInfo(pClass->m_iPad,&UIScene_LoadCreateJoinMenu::CrossSaveGetSavesInfoCallback,pClass,"save");

				pClass->m_eSaveTransferState = eSaveTransfer_GettingSavesInfo;

			}

			break;

		case eSaveTransfer_GettingSavesInfo:

			if(pClass->m_saveTransferDownloadCancelled)

			{

				WCHAR wcTemp[256];

				swprintf(wcTemp,256, app.GetString(IDS_CANCEL));		// MGH - should change this string to "cancelling download"

				m_wstrStageText=wcTemp;

				pMinecraft->progressRenderer->progressStage( m_wstrStageText );

			}

			break;



		case eSaveTransfer_GetFileData:

		{

			bSaveFileCreated = true;

			StorageManager.GetSaveUniqueFileDir(pClass->m_downloadedUniqueFilename);



			if(pClass->m_saveTransferDownloadCancelled)

			{

				pClass->m_eSaveTransferState = eSaveTransfer_Error;

				break;

			}

			PSAVE_DETAILS pSaveDetails=StorageManager.ReturnSavesInfo();

			int idx = pClass->m_iSaveListIndex;

			app.getRemoteStorage()->waitForStorageManagerIdle();

			bool bGettingOK = app.getRemoteStorage()->getSaveData(pClass->m_downloadedUniqueFilename, SaveTransferReturned, pClass);

			if(bGettingOK)

			{

				pClass->m_eSaveTransferState = eSaveTransfer_GettingFileData;

			}

			else

			{

				pClass->m_eSaveTransferState = eSaveTransfer_Error;

				app.DebugPrintf("app.getRemoteStorage()->getSaveData failed\n");



			}

		}



        case eSaveTransfer_GettingFileData:

            {

                WCHAR wcTemp[256];



                int dataProgress = app.getRemoteStorage()->getDataProgress();

                pMinecraft->progressRenderer->progressStagePercentage(dataProgress);



                //swprintf(wcTemp, 256, L"Downloading data : %d", dataProgress);//app.GetString(IDS_SAVETRANSFER_STAGE_GET_DATA),0,pClass->m_ulFileSize);

                swprintf(wcTemp,256, app.GetString(IDS_SAVETRANSFER_STAGE_GET_DATA),dataProgress);

                m_wstrStageText=wcTemp;

                pMinecraft->progressRenderer->progressStage( m_wstrStageText );

				if(pClass->m_saveTransferDownloadCancelled && bAbortCalled == false)

				{

					app.getRemoteStorage()->abort();

					bAbortCalled = true;

				}

            }

            break;

        case eSaveTransfer_FileDataRetrieved:

			pClass->m_eSaveTransferState = eSaveTransfer_LoadSaveFromDisc;

		break;

		case eSaveTransfer_LoadSaveFromDisc:

		{

			if(pClass->m_saveTransferDownloadCancelled)

			{

				pClass->m_eSaveTransferState = eSaveTransfer_Error;

				break;

			}



			PSAVE_DETAILS pSaveDetails=StorageManager.ReturnSavesInfo();

			int saveInfoIndex = -1;

			for(int i=0;i<pSaveDetails->iSaveC;i++)

			{

				if(strcmp(pSaveDetails->SaveInfoA[i].UTF8SaveFilename, pClass->m_downloadedUniqueFilename) == 0)

				{

					//found it

					saveInfoIndex = i;

				}

			}

			if(saveInfoIndex == -1)

			{

				pClass->m_eSaveTransferState = eSaveTransfer_Error;

				app.DebugPrintf("CrossSaveGetSavesInfoCallback failed - couldn't find save\n");

			}

			else

			{

#ifdef __PS3__

				// ignore the CRC on PS3

				C4JStorage::ESaveGameState eLoadStatus=StorageManager.LoadSaveData(&pSaveDetails->SaveInfoA[saveInfoIndex],&LoadCrossSaveDataCallback,pClass, true);

#else

				C4JStorage::ESaveGameState eLoadStatus=StorageManager.LoadSaveData(&pSaveDetails->SaveInfoA[saveInfoIndex],&LoadCrossSaveDataCallback,pClass);

#endif

				if(eLoadStatus == C4JStorage::ESaveGame_Load)

				{

					pClass->m_eSaveTransferState = eSaveTransfer_LoadingSaveFromDisc;

				}

				else

				{

					pClass->m_eSaveTransferState = eSaveTransfer_Error;

				}

			}

		}

		break;

		case eSaveTransfer_LoadingSaveFromDisc:



			break;

        case eSaveTransfer_CreatingNewSave:

			{

				unsigned int fileSize = StorageManager.GetSaveSize();

				byteArray ba(fileSize);

				StorageManager.GetSaveData(ba.data, &fileSize);

				assert(ba.length == fileSize);





                StorageManager.ResetSaveData();

				{

					PBYTE pbThumbnailData=nullptr;

					DWORD dwThumbnailDataSize=0;



					PBYTE pbDataSaveImage=nullptr;

					DWORD dwDataSizeSaveImage=0;



					StorageManager.GetDefaultSaveImage(&pbDataSaveImage, &dwDataSizeSaveImage);

				if(app.getRemoteStorage()->getThumbnailData() != nullptr && app.getRemoteStorage()->getThumbnailDataSize() > 0)
				{
					dwThumbnailDataSize = app.getRemoteStorage()->getThumbnailDataSize();
					pbThumbnailData = new BYTE[dwThumbnailDataSize];
					memcpy(pbThumbnailData, app.getRemoteStorage()->getThumbnailData(), dwThumbnailDataSize);
				}
				else
				{
					StorageManager.GetDefaultSaveThumbnail(&pbThumbnailData,&dwThumbnailDataSize);
				}



					BYTE bTextMetadata[88];

					ZeroMemory(bTextMetadata,88);

					unsigned int remoteHostOptions = app.getRemoteStorage()->getSaveHostOptions();

					app.SetGameHostOption(eGameHostOption_All, remoteHostOptions );

					int iTextMetadataBytes = app.CreateImageTextData(bTextMetadata, app.getRemoteStorage()->getSaveSeed(), true, remoteHostOptions, app.getRemoteStorage()->getSaveTexturePack() );



					// set the icon and save image

					StorageManager.SetSaveImages(pbThumbnailData,dwThumbnailDataSize,pbDataSaveImage,dwDataSizeSaveImage,bTextMetadata,iTextMetadataBytes);

				}





#ifdef SPLIT_SAVES

                ConsoleSaveFileOriginal oldFormatSave( wSaveName, ba.data, ba.length, false, app.getRemoteStorage()->getSavePlatform() );

                pSave = new ConsoleSaveFileSplit( &oldFormatSave, false, pMinecraft->progressRenderer );



                pMinecraft->progressRenderer->progressStage(IDS_SAVETRANSFER_STAGE_SAVING);

                pSave->Flush(false,false);

                pClass->m_eSaveTransferState = eSaveTransfer_Saving;

#else

                pSave = new ConsoleSaveFileOriginal( wSaveName, ba.data, ba.length, false, app.getRemoteStorage()->getSavePlatform() );

                pClass->m_eSaveTransferState = eSaveTransfer_Converting;

				pMinecraft->progressRenderer->progressStage(IDS_SAVETRANSFER_STAGE_CONVERTING);

#endif

                delete ba.data;

            }

			break;

        case eSaveTransfer_Converting:

			{

            pSave->ConvertToLocalPlatform(); // check if we need to convert this file from PS3->PS4

            pClass->m_eSaveTransferState = eSaveTransfer_Saving;

            pMinecraft->progressRenderer->progressStage(IDS_SAVETRANSFER_STAGE_SAVING);

			StorageManager.SetSaveTitle(wSaveName);

			StorageManager.SetSaveUniqueFilename(pClass->m_downloadedUniqueFilename);



			app.getRemoteStorage()->waitForStorageManagerIdle();	// we need to wait for the save system to be idle here, as Flush doesn't check for it.

            pSave->Flush(false, false);

			}

            break;

        case eSaveTransfer_Saving:

			{

				// On Durango/Orbis, we need to wait for all the asynchronous saving processes to complete before destroying the levels, as that will ultimately delete

				// the directory level storage & therefore the ConsoleSaveSplit instance, which needs to be around until all the sub files have completed saving.

#if defined(_DURANGO) || defined(__ORBIS__)

				while(StorageManager.GetSaveState() != C4JStorage::ESaveGame_Idle )

				{

					Sleep(10);

					StorageManager.Tick();

				}

#endif



				delete pSave;





				pMinecraft->progressRenderer->progressStage(IDS_PROGRESS_SAVING_TO_DISC);

				pClass->m_eSaveTransferState = eSaveTransfer_Succeeded;

			}

            break;



		case eSaveTransfer_Succeeded:

			{

				// if we've arrived here, the save has been created successfully

				pClass->m_iState=e_SavesRepopulate;

				pClass->updateTooltips();

				UINT uiIDA[1];

				uiIDA[0]=IDS_CONFIRM_OK;

				app.getRemoteStorage()->waitForStorageManagerIdle();	// wait for everything to complete before we hand control back to the player

				ui.RequestErrorMessage( IDS_TOOLTIPS_SAVETRANSFER_DOWNLOAD, IDS_SAVE_TRANSFER_DOWNLOADCOMPLETE, uiIDA,1,ProfileManager.GetPrimaryPad(),CrossSaveFinishedCallback,pClass);

				pClass->m_eSaveTransferState = eSaveTransfer_Finished;

			}

			break;



		case eSaveTransfer_Cancelled: // this is no longer used

			{

				assert(0); //pClass->m_eSaveTransferState = eSaveTransfer_Idle;

			}

			break;

        case eSaveTransfer_Error:

			{

				if(bSaveFileCreated)

				{

					if(pClass->m_saveTransferDownloadCancelled)

					{

						WCHAR wcTemp[256];

					swprintf(wcTemp,256, app.GetString(IDS_CANCEL));		// MGH - should change this string to "cancelling download"

						m_wstrStageText=wcTemp;

						pMinecraft->progressRenderer->progressStage( m_wstrStageText );

						pMinecraft->progressRenderer->progressStage( m_wstrStageText );

					}

					// if the save file has already been created we have to delete it again if there's been an error

					PSAVE_DETAILS pSaveDetails=StorageManager.ReturnSavesInfo();

					int saveInfoIndex = -1;

					for(int i=0;i<pSaveDetails->iSaveC;i++)

					{

						if(strcmp(pSaveDetails->SaveInfoA[i].UTF8SaveFilename, pClass->m_downloadedUniqueFilename) == 0)

						{

							//found it

							saveInfoIndex = i;

						}

					}

					if(saveInfoIndex == -1)

					{

						app.DebugPrintf("eSaveTransfer_Error failed - couldn't find save\n");

						assert(0);

						pClass->m_eSaveTransferState = eSaveTransfer_ErrorMesssage;

					}

					else

					{

						// delete the save file

						app.getRemoteStorage()->waitForStorageManagerIdle();

							C4JStorage::ESaveGameState eDeleteStatus = StorageManager.DeleteSaveData(&pSaveDetails->SaveInfoA[saveInfoIndex],UIScene_LoadCreateJoinMenu::CrossSaveDeleteOnErrorReturned,pClass);

						if(eDeleteStatus == C4JStorage::ESaveGame_Delete)

						{

							pClass->m_eSaveTransferState = eSaveTransfer_ErrorDeletingSave;

						}

						else

						{

							app.DebugPrintf("StorageManager.DeleteSaveData failed!!\n");

							pClass->m_eSaveTransferState = eSaveTransfer_ErrorMesssage;

						}

					}

				}

				else

				{

					pClass->m_eSaveTransferState = eSaveTransfer_ErrorMesssage;

				}

			}

            break;



		case eSaveTransfer_ErrorDeletingSave:

			break;

		case eSaveTransfer_ErrorMesssage:

			{

				app.getRemoteStorage()->waitForStorageManagerIdle();	// wait for everything to complete before we hand control back to the player

				if(pClass->m_saveTransferDownloadCancelled)

				{

					pClass->m_eSaveTransferState = eSaveTransfer_Idle;

				}

				else

				{

					UINT uiIDA[1];

					uiIDA[0]=IDS_CONFIRM_OK;

					UINT errorMessage = IDS_SAVE_TRANSFER_DOWNLOADFAILED;

					if(!ProfileManager.IsSignedInLive(ProfileManager.GetPrimaryPad()))

					{

						errorMessage = IDS_ERROR_NETWORK;			// show "A network error has occurred."

#ifdef __ORBIS__

						if(!ProfileManager.isSignedInPSN(ProfileManager.GetPrimaryPad()))

						{

							errorMessage = IDS_PRO_NOTONLINE_TEXT;		// show "not signed into PSN"

						}

#endif

#ifdef __VITA__

						if(!ProfileManager.IsSignedInPSN(ProfileManager.GetPrimaryPad()))

						{

							errorMessage = IDS_PRO_NOTONLINE_TEXT;		// show "not signed into PSN"

						}

#endif



					}

					ui.RequestErrorMessage( IDS_TOOLTIPS_SAVETRANSFER_DOWNLOAD, errorMessage, uiIDA,1,ProfileManager.GetPrimaryPad(),CrossSaveFinishedCallback,pClass);

					pClass->m_eSaveTransferState = eSaveTransfer_Finished;

				}

				if(bSaveFileCreated)		// save file has been created, then deleted.

					pClass->m_iState=e_SavesRepopulateAfterDelete;

				else

					pClass->m_iState=e_SavesRepopulate;

				pClass->updateTooltips();

			}

			break;

		case eSaveTransfer_Finished:

			{

			}

			// waiting to dismiss the dialog

			break;

        }

        Sleep(50);

    }

	m_bSaveTransferRunning = false;

#ifdef __PS3__

	StorageManager.SetSaveTransferInProgress(false);

#endif

    return 0;



}



void UIScene_LoadCreateJoinMenu::SaveTransferReturned(LPVOID lpParam, SonyRemoteStorage::Status s, int error_code)

{

    UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu *) lpParam;



    if(s == SonyRemoteStorage::e_getDataSucceeded)

	{

        pClass->m_eSaveTransferState = eSaveTransfer_FileDataRetrieved;

	}

    else

	{

        pClass->m_eSaveTransferState = eSaveTransfer_Error;

		app.DebugPrintf("SaveTransferReturned failed with error code : 0x%08x\n", error_code);

	}



}

ConsoleSaveFile* UIScene_LoadCreateJoinMenu::SonyCrossSaveConvert()

{

    return nullptr;

}



void UIScene_LoadCreateJoinMenu::CancelSaveTransferCallback(LPVOID lpParam)

{

    UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu *) lpParam;

	pClass->m_saveTransferDownloadCancelled = true;

	ui.SetTooltips( DEFAULT_XUI_MENU_USER, -1, -1, -1, -1,-1,-1,-1,-1);		// MGH -  added - remove the "cancel" tooltip, so the player knows it's underway (really needs a "cancelling" message)

}



#endif







#ifdef SONY_REMOTE_STORAGE_UPLOAD



void UIScene_LoadCreateJoinMenu::LaunchSaveUpload()

{

    LoadingInputParams *loadingParams = new LoadingInputParams();

    loadingParams->func = &UIScene_LoadCreateJoinMenu::UploadSonyCrossSaveThreadProc;

    loadingParams->lpParam = (LPVOID)this;



    UIFullscreenProgressCompletionData *completionData = new UIFullscreenProgressCompletionData();

    completionData->bShowBackground=TRUE;

    completionData->bShowLogo=TRUE;

    completionData->type = e_ProgressCompletion_NavigateBackToScene;

    completionData->iPad = DEFAULT_XUI_MENU_USER;

    loadingParams->completionData = completionData;



// 4J-PB - Waiting for Sony to fix canceling a save upload

	loadingParams->cancelFunc=&UIScene_LoadCreateJoinMenu::CancelSaveUploadCallback;

	loadingParams->m_cancelFuncParam = this;

 loadingParams->cancelText=IDS_TOOLTIPS_CANCEL;



    ui.NavigateToScene(m_iPad,eUIScene_FullscreenProgress, loadingParams);



}



int UIScene_LoadCreateJoinMenu::CrossSaveUploadFinishedCallback(void *pParam,int iPad,C4JStorage::EMessageResult result)

{

	UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu *) pParam;

	pClass->m_eSaveUploadState = eSaveUpload_Idle;



	return 0;

}





int UIScene_LoadCreateJoinMenu::UploadSonyCrossSaveThreadProc( LPVOID lpParameter )

{

    UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu *) lpParameter;

	pClass->m_saveTransferUploadCancelled = false;

	bool bAbortCalled = false;

    Minecraft *pMinecraft=Minecraft::GetInstance();



    // get the save file size

	pMinecraft->progressRenderer->progressStagePercentage(0);

    pMinecraft->progressRenderer->progressStart(IDS_TOOLTIPS_SAVETRANSFER_UPLOAD);

    pMinecraft->progressRenderer->progressStage( IDS_TOOLTIPS_SAVETRANSFER_UPLOAD );



    PSAVE_DETAILS pSaveDetails=StorageManager.ReturnSavesInfo();

    int idx = pClass->m_iSaveListIndex;

    bool bSettingOK = app.getRemoteStorage()->setSaveData(&pSaveDetails->SaveInfoA[idx], SaveUploadReturned, pClass);



	if(bSettingOK)

    {

        pClass->m_eSaveUploadState = eSaveUpload_UploadingFileData;

        pMinecraft->progressRenderer->progressStagePercentage(0);

    }

    else

	{

        pClass->m_eSaveUploadState = eSaveUpload_Error;

	}



    while(pClass->m_eSaveUploadState!=eSaveUpload_Idle)

    {

        switch(pClass->m_eSaveUploadState)

        {

        case eSaveUpload_Idle:

            break;

        case eSaveUpload_UploadingFileData:

            {

                WCHAR wcTemp[256];

                int dataProgress = app.getRemoteStorage()->getDataProgress();

                pMinecraft->progressRenderer->progressStagePercentage(dataProgress);



                //swprintf(wcTemp, 256, L"Uploading data : %d", dataProgress);//app.GetString(IDS_SAVETRANSFER_STAGE_GET_DATA),0,pClass->m_ulFileSize);

                swprintf(wcTemp,256, app.GetString(IDS_SAVETRANSFER_STAGE_PUT_DATA),dataProgress);



                m_wstrStageText=wcTemp;

                pMinecraft->progressRenderer->progressStage( m_wstrStageText );

// 4J-PB - Waiting for Sony to fix canceling a save upload

				if(pClass->m_saveTransferUploadCancelled && bAbortCalled == false)

				{

					// we only really want to be able to cancel during the download of data, if it's taking a long time

					app.getRemoteStorage()->abort();

					bAbortCalled = true;

				}

            }

            break;

		case eSaveUpload_FileDataUploaded:

			{

				UINT uiIDA[1];

				uiIDA[0]=IDS_CONFIRM_OK;

				ui.RequestErrorMessage( IDS_TOOLTIPS_SAVETRANSFER_UPLOAD, IDS_SAVE_TRANSFER_UPLOADCOMPLETE, uiIDA,1,ProfileManager.GetPrimaryPad(),CrossSaveUploadFinishedCallback,pClass);

				pClass->m_eSaveUploadState = esaveUpload_Finished;

			}

			break;

		case eSaveUpload_Cancelled: // this is no longer used

			assert(0);//			pClass->m_eSaveUploadState = eSaveUpload_Idle;

			break;

        case eSaveUpload_Error:

			{

				if(pClass->m_saveTransferUploadCancelled)

				{

					pClass->m_eSaveUploadState = eSaveUpload_Idle;

				}

				else

				{

					UINT uiIDA[1];

					uiIDA[0]=IDS_CONFIRM_OK;

					ui.RequestErrorMessage( IDS_TOOLTIPS_SAVETRANSFER_UPLOAD, IDS_SAVE_TRANSFER_UPLOADFAILED, uiIDA,1,ProfileManager.GetPrimaryPad(),CrossSaveUploadFinishedCallback,pClass);

					pClass->m_eSaveUploadState = esaveUpload_Finished;

				}

			}

            break;

		case esaveUpload_Finished:

			// waiting for dialog to be dismissed

			break;

        }

        Sleep(50);

    }



    return 0;



}



void UIScene_LoadCreateJoinMenu::SaveUploadReturned(LPVOID lpParam, SonyRemoteStorage::Status s, int error_code)

{

    UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu *) lpParam;



	if(pClass->m_saveTransferUploadCancelled)

	{

		UINT uiIDA[1] = { IDS_CONFIRM_OK };

		ui.RequestErrorMessage( IDS_CANCEL_UPLOAD_TITLE, IDS_CANCEL_UPLOAD_TEXT, uiIDA, 1, ProfileManager.GetPrimaryPad(), CrossSaveUploadFinishedCallback, pClass );

		pClass->m_eSaveUploadState=esaveUpload_Finished;

	}

	else

    {

		if(s == SonyRemoteStorage::e_setDataSucceeded)

		   pClass->m_eSaveUploadState = eSaveUpload_FileDataUploaded;

		else if ( !pClass->m_saveTransferUploadCancelled )

			pClass->m_eSaveUploadState = eSaveUpload_Error;

	}

}



void UIScene_LoadCreateJoinMenu::CancelSaveUploadCallback(LPVOID lpParam)

{

    UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu *) lpParam;

	pClass->m_saveTransferUploadCancelled = true;

	app.DebugPrintf("m_saveTransferUploadCancelled = true\n");

	ui.SetTooltips( DEFAULT_XUI_MENU_USER, -1, -1, -1, -1,-1,-1,-1,-1);		// MGH -  added - remove the "cancel" tooltip, so the player knows it's underway (really needs a "cancelling" message)



	pClass->m_bIgnoreInput = true;

}



int UIScene_LoadCreateJoinMenu::SaveTransferDialogReturned(void *pParam,int iPad,C4JStorage::EMessageResult result)

{

	UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu*)pParam;

	// results switched for this dialog

	if(result==C4JStorage::EMessage_ResultAccept)

	{

		// upload the save

		pClass->LaunchSaveUpload();



		pClass->m_bIgnoreInput=false;

	}

	else

	{

		pClass->m_bIgnoreInput=false;

	}

	return 0;

}

#endif // SONY_REMOTE_STORAGE_UPLOAD





#if defined _XBOX_ONE

void UIScene_LoadCreateJoinMenu::LaunchSaveTransfer()

{

	SaveTransferStateContainer *stateContainer = new SaveTransferStateContainer();

	stateContainer->m_iProgress = 0;

	stateContainer->m_bSaveTransferInProgress = false;

	stateContainer->m_bSaveTransferCancelled = false;

	stateContainer->m_iPad = m_iPad;

	stateContainer->m_eSaveTransferState = C4JStorage::eSaveTransfer_Idle;

	stateContainer->m_pClass = this;



    LoadingInputParams *loadingParams = new LoadingInputParams();

    loadingParams->func = &UIScene_LoadCreateJoinMenu::DownloadXbox360SaveThreadProc;

    loadingParams->lpParam = (LPVOID)stateContainer;



    UIFullscreenProgressCompletionData *completionData = new UIFullscreenProgressCompletionData();

    completionData->bShowBackground=TRUE;

    completionData->bShowLogo=TRUE;

    completionData->type = e_ProgressCompletion_NavigateBackToScene;

    completionData->iPad = DEFAULT_XUI_MENU_USER;

    completionData->bRequiresUserAction=TRUE;

    loadingParams->completionData = completionData;



    loadingParams->cancelFunc=&UIScene_LoadCreateJoinMenu::CancelSaveTransferCallback;

    loadingParams->m_cancelFuncParam=stateContainer;

    loadingParams->cancelText=IDS_TOOLTIPS_CANCEL;



    ui.NavigateToScene(m_iPad,eUIScene_FullscreenProgress, loadingParams);

}







int UIScene_LoadCreateJoinMenu::DownloadXbox360SaveThreadProc( LPVOID lpParameter )

{

    Compression::UseDefaultThreadStorage();



	SaveTransferStateContainer *pStateContainer = (SaveTransferStateContainer *) lpParameter;

    Minecraft *pMinecraft=Minecraft::GetInstance();

    ConsoleSaveFile* pSave = nullptr;



	while(StorageManager.SaveTransferClearState()!=C4JStorage::eSaveTransfer_Idle)

	{

		Sleep(5);

	}



    pStateContainer->m_bSaveTransferInProgress=true;



    UIScene_LoadCreateJoinMenu::s_eSaveTransferFile = eSaveTransferFile_Marker;

    RequestFileSize( pStateContainer, L"completemarker" );



    while((pStateContainer->m_eSaveTransferState!=C4JStorage::eSaveTransfer_Idle) && pStateContainer->m_bSaveTransferInProgress && !pStateContainer->m_bSaveTransferCancelled)

    {

        switch(pStateContainer->m_eSaveTransferState)

        {

        case C4JStorage::eSaveTransfer_Idle:

            break;

        case C4JStorage::eSaveTransfer_FileSizeRetrieved:

            switch(UIScene_LoadCreateJoinMenu::s_eSaveTransferFile)

            {

            case eSaveTransferFile_Marker:

				if(UIScene_LoadCreateJoinMenu::s_ulFileSize == 0)

				{

					pMinecraft->progressRenderer->progressStage(IDS_SAVETRANSFER_NONE_FOUND);

					pStateContainer->m_eSaveTransferState=C4JStorage::eSaveTransfer_Idle;

				}

				else

				{

					RequestFileData( pStateContainer, L"completemarker" );

				}

                break;

            case eSaveTransferFile_Metadata:

                RequestFileData( pStateContainer, L"metadata" );

                break;

            case eSaveTransferFile_SaveData:

                RequestFileData( pStateContainer, L"savedata" );

                break;

            };

            break;

        case C4JStorage::eSaveTransfer_GettingFileData:



            break;

        case C4JStorage::eSaveTransfer_FileDataRetrieved:

            switch(UIScene_LoadCreateJoinMenu::s_eSaveTransferFile)

            {

            case eSaveTransferFile_Marker:

				// MGH - the marker file now contains the save file version number

				// if the version is higher than we handle, cancel the download.

				if(UIScene_LoadCreateJoinMenu::s_transferData[0] > SAVE_FILE_VERSION_NUMBER)

				{

					pMinecraft->progressRenderer->progressStage(IDS_SAVETRANSFER_NONE_FOUND);

					pStateContainer->m_eSaveTransferState=C4JStorage::eSaveTransfer_Idle;

				}

				else

                {

					UIScene_LoadCreateJoinMenu::s_eSaveTransferFile = eSaveTransferFile_Metadata;

					RequestFileSize( pStateContainer, L"metadata" );

				}

                break;

            case eSaveTransferFile_Metadata:

                {

                    ByteArrayInputStream bais(UIScene_LoadCreateJoinMenu::s_transferData);

                    DataInputStream dis(&bais);



                    wstring saveTitle = dis.readUTF();

                    StorageManager.SetSaveTitle(saveTitle.c_str());



                    wstring saveUniqueName = dis.readUTF();



					// 4J Stu - Don't set this any more. We added it so that we could share the ban list data for this save

					// However if the player downloads the same save multiple times, it will overwrite the previous version

					// with that filname, and they could have made changes to it.

                    //StorageManager.SetSaveUniqueFilename((wchar_t *)saveUniqueName.c_str());



                    int thumbnailSize = dis.readInt();

                    if(thumbnailSize > 0)

                    {

                        byteArray ba(thumbnailSize);

                        dis.readFully(ba);







						// retrieve the seed value from the image metadata, we need to change to host options, then set it back again

						bool bHostOptionsRead = false;

						unsigned int uiHostOptions = 0;

						DWORD dwTexturePack;

						int64_t seedVal;



						char szSeed[50];

						ZeroMemory(szSeed,50);

						app.GetImageTextData(ba.data,ba.length,(unsigned char *)&szSeed,uiHostOptions,bHostOptionsRead,dwTexturePack);

						sscanf_s(szSeed, "%I64d", &seedVal);



						app.SetGameHostOption(uiHostOptions, eGameHostOption_WorldSize, e_worldSize_Classic);		// force the classic world size on, otherwise it's unknown and we can't expand





						BYTE bTextMetadata[88];

						ZeroMemory(bTextMetadata,88);



						int iTextMetadataBytes = app.CreateImageTextData(bTextMetadata, seedVal, true, uiHostOptions, dwTexturePack);

						// set the icon and save image

						StorageManager.SetSaveImages(ba.data, ba.length, nullptr, 0, bTextMetadata, iTextMetadataBytes);



                        delete ba.data;

                    }



                    UIScene_LoadCreateJoinMenu::s_transferData = byteArray();

                    UIScene_LoadCreateJoinMenu::s_eSaveTransferFile = eSaveTransferFile_SaveData;

                    RequestFileSize( pStateContainer, L"savedata" );

                }

                break;

            case eSaveTransferFile_SaveData:

                {

#ifdef SPLIT_SAVES

					if(!pStateContainer->m_bSaveTransferCancelled)

					{

						ConsoleSaveFileOriginal oldFormatSave( L"Temp name", UIScene_LoadCreateJoinMenu::s_transferData.data, UIScene_LoadCreateJoinMenu::s_transferData.length, false, SAVE_FILE_PLATFORM_X360 );

						pSave = new ConsoleSaveFileSplit( &oldFormatSave, false, pMinecraft->progressRenderer );



						pMinecraft->progressRenderer->progressStage(IDS_SAVETRANSFER_STAGE_SAVING);

						if(!pStateContainer->m_bSaveTransferCancelled) pSave->Flush(false,false);

					}

                    pStateContainer->m_eSaveTransferState=C4JStorage::eSaveTransfer_Saving;



#else

                    pSave = new ConsoleSaveFileOriginal( wSaveName, m_transferData.data, m_transferData.length, false, SAVE_FILE_PLATFORM_X360 );

                    pStateContainer->m_eSaveTransferState=C4JStorage::eSaveTransfer_Converting;

#endif

                    delete UIScene_LoadCreateJoinMenu::s_transferData.data;

                    UIScene_LoadCreateJoinMenu::s_transferData = byteArray();

                }

                break;

            };



            pStateContainer->m_iProgress=0;

            break;

        case C4JStorage::eSaveTransfer_Converting:

#if 0

            pSave->ConvertToLocalPlatform();



            pMinecraft->progressRenderer->progressStage(IDS_SAVETRANSFER_STAGE_SAVING);

            if(!pStateContainer->m_bSaveTransferCancelled) pSave->Flush(false,false);



            pStateContainer->m_iProgress+=1;

            if(pStateContainer->m_iProgress==101)

            {

                pStateContainer->m_eSaveTransferState=C4JStorage::eSaveTransfer_Saving;

                pStateContainer->m_iProgress=0;

                break;

            }

            pMinecraft->progressRenderer->progressStagePercentage(pStateContainer->m_iProgress);

#endif

            break;

        case C4JStorage::eSaveTransfer_Saving:

            // On Durango/Orbis, we need to wait for all the asynchronous saving processes to complete before destroying the levels, as that will ultimately delete

            // the directory level storage & therefore the ConsoleSaveSplit instance, which needs to be around until all the sub files have completed saving.

#if defined(_DURANGO) || defined(__ORBIS__)

			pMinecraft->progressRenderer->progressStage(IDS_PROGRESS_SAVING_TO_DISC);



			while(StorageManager.GetSaveState() != C4JStorage::ESaveGame_Idle )

            {

                Sleep(10);



				// 4J Stu - DO NOT tick this here. The main thread should be the only place ticking the StorageManager. You WILL get crashes.

                //StorageManager.Tick();

            }

#endif



            delete pSave;



#ifdef _XBOX_ONE

			pMinecraft->progressRenderer->progressStage(IDS_SAVE_TRANSFER_DOWNLOAD_AND_CONVERT_COMPLETE);

#endif



            pStateContainer->m_eSaveTransferState=C4JStorage::eSaveTransfer_Idle;



			 // wipe the list and repopulate it

			 if(!pStateContainer->m_bSaveTransferCancelled) pStateContainer->m_pClass->m_iState=e_SavesRepopulateAfterTransferDownload;



            //pClass->m_iProgress+=1;

            //if(pClass->m_iProgress==101)

            //{

            //	pClass->m_iProgress=0;

            //	pClass->m_eSaveTransferState=C4JStorage::eSaveTransfer_Idle;

            //	pMinecraft->progressRenderer->progressStage( IDS_SAVE_TRANSFER_DOWNLOAD_AND_CONVERT_COMPLETE );



            //	break;

            //}

            //pMinecraft->progressRenderer->progressStagePercentage(pClass->m_iProgress);



            break;

        }

        Sleep(50);

    }



    if(pStateContainer->m_bSaveTransferCancelled)

    {

        WCHAR wcTemp[256];



        pStateContainer->m_bSaveTransferCancelled=false;

        swprintf(wcTemp,app.GetString(IDS_SAVE_TRANSFER_DOWNLOAD_CANCELLED));

        m_wstrStageText=wcTemp;

        pMinecraft->progressRenderer->progressStage( m_wstrStageText );



    }



    pStateContainer->m_eSaveTransferState=C4JStorage::eSaveTransfer_Idle;

    pStateContainer->m_bSaveTransferInProgress=false;



	delete pStateContainer;



    return 0;

}



void UIScene_LoadCreateJoinMenu::RequestFileSize( SaveTransferStateContainer *pClass, wchar_t *filename )

{

    Minecraft *pMinecraft=Minecraft::GetInstance();



    // get the save file size

    pMinecraft->progressRenderer->progressStart(IDS_SAVETRANSFER_TITLE_GET);

    pMinecraft->progressRenderer->progressStage( IDS_SAVETRANSFER_STAGE_GET_DETAILS );



#ifdef _DEBUG_MENUS_ENABLED

    if(app.GetLoadSavesFromFolderEnabled())

    {

        ZeroMemory(&m_debugTransferDetails, sizeof(C4JStorage::SAVETRANSFER_FILE_DETAILS) );



        File targetFile( wstring(L"FakeTMSPP\\").append(filename) );

        if(targetFile.exists()) m_debugTransferDetails.ulFileLen = targetFile.length();



        SaveTransferReturned(pClass,&m_debugTransferDetails);

    }

    else

#endif

    {

        do

		{

			pMinecraft->progressRenderer->progressStart(IDS_SAVETRANSFER_TITLE_GET);

			pMinecraft->progressRenderer->progressStage( IDS_SAVETRANSFER_STAGE_GET_DETAILS );

			Sleep(1);

			pClass->m_eSaveTransferState=StorageManager.SaveTransferGetDetails(pClass->m_iPad,C4JStorage::eGlobalStorage_TitleUser,filename,&UIScene_LoadCreateJoinMenu::SaveTransferReturned,pClass);

		}

		while(pClass->m_eSaveTransferState == C4JStorage::eSaveTransfer_Busy && !pClass->m_bSaveTransferCancelled );

    }

}



void UIScene_LoadCreateJoinMenu::RequestFileData( SaveTransferStateContainer *pClass, wchar_t *filename )

{

    Minecraft *pMinecraft=Minecraft::GetInstance();

    WCHAR wcTemp[256];



    pMinecraft->progressRenderer->progressStagePercentage(0);



    swprintf(wcTemp,app.GetString(IDS_SAVETRANSFER_STAGE_GET_DATA),0,UIScene_LoadCreateJoinMenu::s_ulFileSize);

    m_wstrStageText=wcTemp;



    pMinecraft->progressRenderer->progressStage( m_wstrStageText );



#ifdef _DEBUG_MENUS_ENABLED

    if(app.GetLoadSavesFromFolderEnabled())

    {

        File targetFile( wstring(L"FakeTMSPP\\").append(filename) );

        if(targetFile.exists())

        {

            HANDLE hSaveFile = CreateFile( targetFile.getPath().c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, nullptr);



            m_debugTransferDetails.pbData = new BYTE[m_debugTransferDetails.ulFileLen];



            DWORD numberOfBytesRead = 0;

            ReadFile( hSaveFile,m_debugTransferDetails.pbData,m_debugTransferDetails.ulFileLen,&numberOfBytesRead,nullptr);

            assert(numberOfBytesRead == m_debugTransferDetails.ulFileLen);



            CloseHandle(hSaveFile);



            SaveTransferReturned(pClass,&m_debugTransferDetails);

        }

    }

    else

#endif

    {

        do

		{

			pMinecraft->progressRenderer->progressStart(IDS_SAVETRANSFER_TITLE_GET);

			pMinecraft->progressRenderer->progressStage( -1 );

			Sleep(1);

			pClass->m_eSaveTransferState=StorageManager.SaveTransferGetData(pClass->m_iPad,C4JStorage::eGlobalStorage_TitleUser,filename,&UIScene_LoadCreateJoinMenu::SaveTransferReturned,&UIScene_LoadCreateJoinMenu::SaveTransferUpdateProgress,pClass,pClass);

		}

		while(pClass->m_eSaveTransferState == C4JStorage::eSaveTransfer_Busy && !pClass->m_bSaveTransferCancelled );

    }

}



int UIScene_LoadCreateJoinMenu::SaveTransferReturned(LPVOID lpParam,C4JStorage::SAVETRANSFER_FILE_DETAILS *pSaveTransferDetails)

{

    SaveTransferStateContainer* pClass = (SaveTransferStateContainer *) lpParam;

    app.DebugPrintf("Save Transfer - size is %d\n",pSaveTransferDetails->ulFileLen);



    // if the file data is null, then assume this is the file size retrieval

    if(pSaveTransferDetails->pbData==nullptr)

    {

        pClass->m_eSaveTransferState=C4JStorage::eSaveTransfer_FileSizeRetrieved;

        UIScene_LoadCreateJoinMenu::s_ulFileSize=pSaveTransferDetails->ulFileLen;

    }

    else

    {

        delete UIScene_LoadCreateJoinMenu::s_transferData.data;

        UIScene_LoadCreateJoinMenu::s_transferData = byteArray(pSaveTransferDetails->pbData, UIScene_LoadCreateJoinMenu::s_ulFileSize);

        pClass->m_eSaveTransferState=C4JStorage::eSaveTransfer_FileDataRetrieved;

    }



    return 0;

}



int UIScene_LoadCreateJoinMenu::SaveTransferUpdateProgress(LPVOID lpParam,unsigned long ulBytesReceived)

{

    WCHAR wcTemp[256];



    SaveTransferStateContainer* pClass = (SaveTransferStateContainer *) lpParam;

    Minecraft *pMinecraft=Minecraft::GetInstance();



    if(pClass->m_bSaveTransferCancelled) // was cancelled

    {

        pMinecraft->progressRenderer->progressStage(IDS_SAVE_TRANSFER_DOWNLOAD_CANCELLING);

        swprintf(wcTemp,app.GetString(IDS_SAVE_TRANSFER_DOWNLOAD_CANCELLING));

        m_wstrStageText=wcTemp;

        pMinecraft->progressRenderer->progressStage( m_wstrStageText );

    }

    else

    {

        unsigned int uiProgress=(unsigned int)(((float)ulBytesReceived/float(UIScene_LoadCreateJoinMenu::s_ulFileSize))*100.0f);



        pMinecraft->progressRenderer->progressStagePercentage(uiProgress);

        swprintf(wcTemp,app.GetString(IDS_SAVETRANSFER_STAGE_GET_DATA),((float)(ulBytesReceived))/1024000.0f,((float)UIScene_LoadCreateJoinMenu::s_ulFileSize)/1024000.0f);

        m_wstrStageText=wcTemp;

        pMinecraft->progressRenderer->progressStage( m_wstrStageText );

    }



    return 0;

}



void UIScene_LoadCreateJoinMenu::CancelSaveTransferCallback(LPVOID lpParam)

{

    SaveTransferStateContainer* pClass = (SaveTransferStateContainer *) lpParam;



    if(!pClass->m_bSaveTransferCancelled)

    {

        StorageManager.CancelSaveTransfer(UIScene_LoadCreateJoinMenu::CancelSaveTransferCompleteCallback,pClass);



        pClass->m_bSaveTransferCancelled=true;

    }

    //pClass->m_bSaveTransferInProgress=false;

}



int UIScene_LoadCreateJoinMenu::CancelSaveTransferCompleteCallback(LPVOID lpParam)

{

    SaveTransferStateContainer* pClass = (SaveTransferStateContainer *) lpParam;

    // change the state to idle to get the download thread to terminate

    pClass->m_eSaveTransferState=C4JStorage::eSaveTransfer_Idle;

    return 0;

}



int UIScene_LoadCreateJoinMenu::NeedSyncMessageReturned(void *pParam,int iPad,C4JStorage::EMessageResult result)

{

    UIScene_LoadCreateJoinMenu *pClass = (UIScene_LoadCreateJoinMenu *)pParam;

    LoadMenuInitData *params = (LoadMenuInitData *)pParam;



    if( result == C4JStorage::EMessage_ResultAccept )

    {

        // navigate to the settings scene

        ui.NavigateToScene(pClass->m_iPad, eUIScene_LoadMenu, pClass->m_loadMenuInitData);

    }

    else

    {

        delete pClass->m_loadMenuInitData;

        pClass->m_bIgnoreInput = false;

    }



    return 0;

}





#endif





#ifdef _XBOX_ONE

void UIScene_LoadCreateJoinMenu::HandleDLCLicenseChange()

{

	// may have installed Halloween on this menu

	app.StartInstallDLCProcess(m_iPad);

}

#endif



#if defined _XBOX_ONE || defined __ORBIS__

int UIScene_LoadCreateJoinMenu::CopySaveDialogReturned(void *pParam,int iPad,C4JStorage::EMessageResult result)

{

    UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu*)pParam;



	if(result==C4JStorage::EMessage_ResultAccept)

	{



		LoadingInputParams *loadingParams = new LoadingInputParams();

		void *uniqueId = (LPVOID)pClass->GetCallbackUniqueId();

		loadingParams->func = &UIScene_LoadCreateJoinMenu::CopySaveThreadProc;

		loadingParams->lpParam = uniqueId;

		loadingParams->waitForThreadToDelete = true;



		UIFullscreenProgressCompletionData *completionData = new UIFullscreenProgressCompletionData();

		completionData->bShowBackground=TRUE;

		completionData->bShowLogo=TRUE;

		completionData->type = e_ProgressCompletion_NavigateBackToScene;

		completionData->iPad = DEFAULT_XUI_MENU_USER;

		loadingParams->completionData = completionData;



		loadingParams->cancelFunc=&UIScene_LoadCreateJoinMenu::CancelCopySaveCallback;

		loadingParams->m_cancelFuncParam=uniqueId;

		loadingParams->cancelText=IDS_TOOLTIPS_CANCEL;



		ui.NavigateToScene(iPad,eUIScene_FullscreenProgress, loadingParams);

	}

	else

	{

		pClass->m_bIgnoreInput=false;

	}



    return 0;

}



int UIScene_LoadCreateJoinMenu::CopySaveThreadProc( LPVOID lpParameter )

{

	Minecraft *pMinecraft=Minecraft::GetInstance();

	pMinecraft->progressRenderer->progressStart(IDS_PROGRESS_COPYING_SAVE);

	pMinecraft->progressRenderer->progressStage( -1 );



	ui.EnterCallbackIdCriticalSection();

	UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu*)ui.GetSceneFromCallbackId((size_t)lpParameter);

	if( pClass )

	{

		pClass->m_bCopying = true;

		pClass->m_bCopyingCancelled = false;

		ui.LeaveCallbackIdCriticalSection();

		// Copy save data takes two callbacks - one for completion, and one for progress. The progress callback also lets us cancel the operation, if we return false.

		StorageManager.CopySaveData(&pClass->m_pSaveDetails->SaveInfoA[pClass->m_iSaveListIndex],UIScene_LoadCreateJoinMenu::CopySaveDataReturned,UIScene_LoadCreateJoinMenu::CopySaveDataProgress,lpParameter);



		bool bContinue = true;

		do

		{

			Sleep(100);

			ui.EnterCallbackIdCriticalSection();

			pClass = (UIScene_LoadCreateJoinMenu*)ui.GetSceneFromCallbackId((size_t)lpParameter);

			if( pClass )

			{

				bContinue = pClass->m_bCopying;

			}

			else

			{

				bContinue = false;

			}

			ui.LeaveCallbackIdCriticalSection();

		} while( bContinue );

	}

	else

	{

		ui.LeaveCallbackIdCriticalSection();

	}



	return 0;

}



int UIScene_LoadCreateJoinMenu::CopySaveDataReturned(LPVOID lpParam, bool success, C4JStorage::ESaveGameState stat)

{

	ui.EnterCallbackIdCriticalSection();

    UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu*)ui.GetSceneFromCallbackId((size_t)lpParam);



	if(pClass)

	{

		if(success)

		{

			pClass->m_bCopying = false;

			// wipe the list and repopulate it

			pClass->m_iState=e_SavesRepopulateAfterDelete;

			ui.LeaveCallbackIdCriticalSection();

		}

		else

		{

#ifdef __ORBIS__

			UINT uiIDA[1];

			// you cancelled the save on exit after choosing exit and save? You go back to the Exit choices then.

			uiIDA[0]=IDS_OK;



			if( stat == C4JStorage::ESaveGame_CopyCompleteFailLocalStorage )

			{

				ui.LeaveCallbackIdCriticalSection();

				ui.RequestErrorMessage(IDS_COPYSAVE_FAILED_TITLE, IDS_COPYSAVE_FAILED_LOCAL, uiIDA, 1, ProfileManager.GetPrimaryPad(), CopySaveErrorDialogFinishedCallback, lpParam);

			}

			else if( stat == C4JStorage::ESaveGame_CopyCompleteFailQuota )

			{

				ui.LeaveCallbackIdCriticalSection();

				ui.RequestErrorMessage(IDS_COPYSAVE_FAILED_TITLE, IDS_COPYSAVE_FAILED_QUOTA, uiIDA, 1, ProfileManager.GetPrimaryPad(), CopySaveErrorDialogFinishedCallback, lpParam);

			}

			else

			{

				pClass->m_bCopying = false;

				ui.LeaveCallbackIdCriticalSection();

			}

#else

			pClass->m_bCopying = false;

			ui.LeaveCallbackIdCriticalSection();

#endif

		}

	}

	else

	{

		ui.LeaveCallbackIdCriticalSection();

	}

	return 0;

}



bool UIScene_LoadCreateJoinMenu::CopySaveDataProgress(LPVOID lpParam, int percent)

{

	bool bContinue = false;

	ui.EnterCallbackIdCriticalSection();

    UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu*)ui.GetSceneFromCallbackId((size_t)lpParam);

	if( pClass )

	{

		bContinue = !pClass->m_bCopyingCancelled;

	}

	ui.LeaveCallbackIdCriticalSection();

	Minecraft *pMinecraft=Minecraft::GetInstance();

	pMinecraft->progressRenderer->progressStagePercentage(percent);



	return bContinue;

}



void UIScene_LoadCreateJoinMenu::CancelCopySaveCallback(LPVOID lpParam)

{

	ui.EnterCallbackIdCriticalSection();

    UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu*)ui.GetSceneFromCallbackId((size_t)lpParam);

	if( pClass )

	{

		pClass->m_bCopyingCancelled = true;

	}

	ui.LeaveCallbackIdCriticalSection();

}



int UIScene_LoadCreateJoinMenu::CopySaveErrorDialogFinishedCallback(void *pParam,int iPad,C4JStorage::EMessageResult result)

{

	ui.EnterCallbackIdCriticalSection();

    UIScene_LoadCreateJoinMenu* pClass = (UIScene_LoadCreateJoinMenu*)ui.GetSceneFromCallbackId((size_t)pParam);

	if( pClass )

	{

		pClass->m_bCopying = false;

	}

	ui.LeaveCallbackIdCriticalSection();



	return 0;

}



#endif // _XBOX_ONE

#ifdef _WINDOWS64

// adding servers bellow



void UIScene_LoadCreateJoinMenu::BeginAddServer()

{

    m_addServerPhase = eAddServer_IP;

    m_addServerIP.clear();

    m_addServerPort.clear();



    UIKeyboardInitData kbData;

    kbData.title       = L"Server Address";

    kbData.defaultText = L"";

    kbData.maxChars    = 128;

    kbData.callback    = &UIScene_LoadCreateJoinMenu::AddServerKeyboardCallback;

    kbData.lpParam     = this;

    kbData.pcMode      = g_KBMInput.IsKBMActive();

    ui.NavigateToScene(m_iPad, eUIScene_Keyboard, &kbData);

}



int UIScene_LoadCreateJoinMenu::AddServerKeyboardCallback(LPVOID lpParam, bool bRes)

{

    UIScene_LoadCreateJoinMenu *pClass = static_cast<UIScene_LoadCreateJoinMenu*>(lpParam);
    if (!bRes)

    {

        pClass->m_addServerPhase = eAddServer_Idle;

        pClass->m_bIgnoreInput = false;

        return 0;

    }



    uint16_t ui16Text[256];

    ZeroMemory(ui16Text, sizeof(ui16Text));

    Win64_GetKeyboardText(ui16Text, 256);



    wchar_t wBuf[256] = {};

    for (int k = 0; k < 255 && ui16Text[k]; k++)

        wBuf[k] = static_cast<wchar_t>(ui16Text[k]);



    if (wBuf[0] == 0)

    {

        pClass->m_addServerPhase = eAddServer_Idle;

        pClass->m_bIgnoreInput = false;

        return 0;

    }



    switch (pClass->m_addServerPhase)

    {

    case eAddServer_IP:

    {

        pClass->m_addServerIP = wBuf;

        pClass->m_addServerPhase = eAddServer_Port;



        UIKeyboardInitData kbData;

        kbData.title       = L"Server Port";

        kbData.defaultText = L"25565";

        kbData.maxChars    = 6;

        kbData.callback    = &UIScene_LoadCreateJoinMenu::AddServerKeyboardCallback;

        kbData.lpParam     = pClass;

        kbData.pcMode      = g_KBMInput.IsKBMActive();

        ui.NavigateToScene(pClass->m_iPad, eUIScene_Keyboard, &kbData);

        break;

    }

    case eAddServer_Port:

    {

        pClass->m_addServerPort = wBuf;

        pClass->m_addServerPhase = eAddServer_Name;



        UIKeyboardInitData kbData;

        kbData.title       = L"Server Name";

        kbData.defaultText = L"Minecraft Server";

        kbData.maxChars    = 64;

        kbData.callback    = &UIScene_LoadCreateJoinMenu::AddServerKeyboardCallback;

        kbData.lpParam     = pClass;

        kbData.pcMode      = g_KBMInput.IsKBMActive();

        ui.NavigateToScene(pClass->m_iPad, eUIScene_Keyboard, &kbData);

        break;

    }

    case eAddServer_Name:

    {

        wstring name = wBuf;

        pClass->AppendServerToFile(pClass->m_addServerIP, pClass->m_addServerPort, name);

        pClass->m_addServerPhase = eAddServer_Idle;

        pClass->m_bIgnoreInput = false;

        g_NetworkManager.ForceFriendsSessionRefresh();

        break;

    }

    default:

        pClass->m_addServerPhase = eAddServer_Idle;

        pClass->m_bIgnoreInput = false;

        break;

    }



    return 0;

}



void UIScene_LoadCreateJoinMenu::AppendServerToFile(const wstring& ip, const wstring& port, const wstring& name)

{

    char narrowIP[256] = {};

    char narrowPort[16] = {};

    char narrowName[256] = {};

    wcstombs(narrowIP, ip.c_str(), sizeof(narrowIP) - 1);

    wcstombs(narrowPort, port.c_str(), sizeof(narrowPort) - 1);

    wcstombs(narrowName, name.c_str(), sizeof(narrowName) - 1);



    uint16_t portNum = static_cast<uint16_t>(atoi(narrowPort));



    struct ServerEntry { std::string ip; uint16_t port; std::string name; };

    std::vector<ServerEntry> entries;



    FILE* file = fopen("servers.db", "rb");

    if (file)

    {

        char magic[4] = {};

        if (fread(magic, 1, 4, file) == 4 && memcmp(magic, "MCSV", 4) == 0)

        {

            uint32_t version = 0, count = 0;

            fread(&version, sizeof(uint32_t), 1, file);

            fread(&count, sizeof(uint32_t), 1, file);

            if (version == 1)

            {

                for (uint32_t s = 0; s < count; s++)

                {

                    uint16_t ipLen = 0, p = 0, nameLen = 0;

                    if (fread(&ipLen, sizeof(uint16_t), 1, file) != 1) break;

                    if (ipLen == 0 || ipLen > 256) break;

                    char ipBuf[257] = {};

                    if (fread(ipBuf, 1, ipLen, file) != ipLen) break;

                    if (fread(&p, sizeof(uint16_t), 1, file) != 1) break;

                    if (fread(&nameLen, sizeof(uint16_t), 1, file) != 1) break;

                    if (nameLen > 256) break;

                    char nameBuf[257] = {};

                    if (nameLen > 0 && fread(nameBuf, 1, nameLen, file) != nameLen) break;

                    entries.push_back({std::string(ipBuf), p, std::string(nameBuf)});

                }

            }

        }

        fclose(file);

    }



    entries.push_back({std::string(narrowIP), portNum, std::string(narrowName)});



    file = fopen("servers.db", "wb");

    if (file)

    {

        fwrite("MCSV", 1, 4, file);

        uint32_t version = 1;

        uint32_t count = static_cast<uint32_t>(entries.size());

        fwrite(&version, sizeof(uint32_t), 1, file);

        fwrite(&count, sizeof(uint32_t), 1, file);



        for (size_t i = 0; i < entries.size(); i++)

        {

            uint16_t ipLen = static_cast<uint16_t>(entries[i].ip.length());

            fwrite(&ipLen, sizeof(uint16_t), 1, file);

            fwrite(entries[i].ip.c_str(), 1, ipLen, file);

            fwrite(&entries[i].port, sizeof(uint16_t), 1, file);

            uint16_t nameLen = static_cast<uint16_t>(entries[i].name.length());

            fwrite(&nameLen, sizeof(uint16_t), 1, file);

            fwrite(entries[i].name.c_str(), 1, nameLen, file);

        }

        fclose(file);

    }

}

#endif // _WINDOWS64




























































































