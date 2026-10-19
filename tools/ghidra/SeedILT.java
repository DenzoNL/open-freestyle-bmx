// Create functions at every incremental-link-table (jmp rel32) target starting at 0x401005
import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;

public class SeedILT extends GhidraScript {
    @Override
    public void run() throws Exception {
        Listing L = currentProgram.getListing();
        Address a = toAddr(0x401005);
        int n = 0, created = 0;
        while (getByte(a) == (byte)0xE9) {
            Address t = a.add(5).add(getInt(a.add(1)));
            n++;
            if (getFunctionAt(t) == null) {
                Data d = L.getDefinedDataContaining(t);
                if (d != null) L.clearCodeUnits(d.getMinAddress(), d.getMaxAddress(), false);
                new DisassembleCommand(t, null, true).applyTo(currentProgram, monitor);
                if (new CreateFunctionCmd(t).applyTo(currentProgram, monitor)) created++;
            }
            a = a.add(5);
        }
        analyzeChanges(currentProgram);
        println("SeedILT stubs " + n + " created " + created);
    }
}
