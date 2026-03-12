#include "stdafx.h"
#include "ClientConstants.h"
#include "Common/BuildVer.h"

const wstring ClientConstants::VERSION_STRING = wstring(L"Minecraft LCE ") + VER_FILEVERSION_STR_W;//+ SharedConstants::VERSION_STRING;
const wstring ClientConstants::BRANCH_STRING = VER_BRANCHVERSION_STR_W;

// Default value for the toggle. If BuildVer defines VER_SHOW_VERSION_WATERMARK, use that.
#ifdef VER_SHOW_VERSION_WATERMARK
const bool ClientConstants::SHOW_VERSION_WATERMARK = (VER_SHOW_VERSION_WATERMARK != 0);
#else
const bool ClientConstants::SHOW_VERSION_WATERMARK = false;
#endif
