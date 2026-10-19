// Apply "addr<TAB>name" labels (e.g. community labels) as function names where a function exists.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;
import java.nio.file.*;

public class ApplyLabels extends GhidraScript {
    @Override
    public void run() throws Exception {
        int n = 0;
        for (String line : Files.readAllLines(Paths.get(getScriptArgs()[0]))) {
            String[] p = line.trim().split("\\s+", 2);
            if (p.length < 2) continue;
            Function f = getFunctionAt(toAddr(Long.parseLong(p[0], 16)));
            if (f == null) continue;
            String name = p[1].replaceAll("[^A-Za-z0-9_]", "_");
            f.setName(name, SourceType.USER_DEFINED);
            n++;
        }
        println("ApplyLabels renamed " + n);
    }
}
