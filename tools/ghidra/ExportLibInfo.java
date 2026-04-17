// Export symbols, functions, and external references from a COFF .lib to a JSON report.
// Designed for headless mode: pass output path as first script argument.
//
// Usage with analyzeHeadless:
//   analyzeHeadless <projDir> <projName> -import <file.lib> \
//     -postScript ExportLibInfo.java <output.json> -deleteProject
//
//@category 4JLibs

import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;

public class ExportLibInfo extends GhidraScript {

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 1) {
            printerr("Usage: ExportLibInfo.java <output.json>");
            return;
        }

        File outputFile = new File(args[0]);
        outputFile.getParentFile().mkdirs();

        PrintWriter pw = new PrintWriter(new FileWriter(outputFile, true));

        String programName = currentProgram.getName();
        Listing listing = currentProgram.getListing();
        SymbolTable symbolTable = currentProgram.getSymbolTable();
        ExternalManager extMgr = currentProgram.getExternalManager();

        // Collect functions
        List<String> functions = new ArrayList<>();
        FunctionIterator funcIter = listing.getFunctions(true);
        while (funcIter.hasNext() && !monitor.isCancelled()) {
            Function f = funcIter.next();
            String sig = f.getPrototypeString(false, false);
            String callingConv = f.getCallingConventionName();
            long size = f.getBody().getNumAddresses();

            functions.add(String.format(
                "    {\"name\": %s, \"entry\": %s, \"signature\": %s, \"callingConvention\": %s, \"size\": %d, \"paramCount\": %d}",
                jsonStr(f.getName()),
                jsonStr(f.getEntryPoint().toString()),
                jsonStr(sig),
                jsonStr(callingConv),
                size,
                f.getParameterCount()
            ));
        }

        // Collect all symbols (non-function)
        List<String> symbols = new ArrayList<>();
        SymbolIterator symIter = symbolTable.getAllSymbols(true);
        while (symIter.hasNext() && !monitor.isCancelled()) {
            Symbol sym = symIter.next();
            if (sym.getSymbolType() == SymbolType.FUNCTION) {
                continue; // already captured above
            }
            if (sym.isExternal()) {
                continue; // captured below
            }
            symbols.add(String.format(
                "    {\"name\": %s, \"type\": %s, \"address\": %s, \"source\": %s}",
                jsonStr(sym.getName(true)),
                jsonStr(sym.getSymbolType().toString()),
                jsonStr(sym.getAddress().toString()),
                jsonStr(sym.getSource().toString())
            ));
        }

        // Collect external symbols (imports from other libraries)
        List<String> externals = new ArrayList<>();
        symIter = symbolTable.getExternalSymbols();
        while (symIter.hasNext() && !monitor.isCancelled()) {
            Symbol sym = symIter.next();
            String extLib = "";
            ExternalLocation extLoc = extMgr.getExternalLocation(sym);
            if (extLoc != null && extLoc.getLibraryName() != null) {
                extLib = extLoc.getLibraryName();
            }
            externals.add(String.format(
                "    {\"name\": %s, \"type\": %s, \"library\": %s}",
                jsonStr(sym.getName(true)),
                jsonStr(sym.getSymbolType().toString()),
                jsonStr(extLib)
            ));
        }

        // Write JSON object for this program/object-file
        pw.println("{");
        pw.println("  \"program\": " + jsonStr(programName) + ",");
        pw.println("  \"language\": " + jsonStr(currentProgram.getLanguageID().toString()) + ",");
        pw.println("  \"compiler\": " + jsonStr(currentProgram.getCompilerSpec().getCompilerSpecID().toString()) + ",");

        pw.println("  \"functionCount\": " + functions.size() + ",");
        pw.println("  \"functions\": [");
        pw.println(String.join(",\n", functions));
        pw.println("  ],");

        pw.println("  \"symbolCount\": " + symbols.size() + ",");
        pw.println("  \"symbols\": [");
        pw.println(String.join(",\n", symbols));
        pw.println("  ],");

        pw.println("  \"externalCount\": " + externals.size() + ",");
        pw.println("  \"externals\": [");
        pw.println(String.join(",\n", externals));
        pw.println("  ]");

        pw.println("}");

        pw.flush();
        pw.close();

        println("ExportLibInfo: wrote " + functions.size() + " functions, " +
                symbols.size() + " symbols, " + externals.size() + " externals for " +
                programName + " -> " + outputFile.getAbsolutePath());
    }

    private String jsonStr(String s) {
        if (s == null) return "null";
        return "\"" + s.replace("\\", "\\\\")
                       .replace("\"", "\\\"")
                       .replace("\n", "\\n")
                       .replace("\r", "\\r")
                       .replace("\t", "\\t") + "\"";
    }
}
