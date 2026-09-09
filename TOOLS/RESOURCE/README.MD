# RESOURCE ARCHIVE TOOLS

`MAKESTUF.EXE` is the surviving DOS resource packer used for the original game. It reads `STUFF.DAT`, the resource files named by that manifest, and `RES_CFG.HPP`, then creates `MAIN.STF`, `MAIN.MAP`, and `MAP.IDX`.

The packer's source code is not present in the recovered repository. Its binary is therefore retained as an essential content-pipeline dependency. Run it through `BUILD/BUILD-RESOURCES.PS1`; the script supplies DOS/4GW and executes it with the bundled DOSBox.

`LEGACY` preserves the original 4DOS gathering scripts, absolute source-file manifests, and distribution helper. They document how `STUFF.DAT` was assembled on Tachyon's development drives, but their hard-coded `I:`, `D:`, and `Y:` paths are not used by the repository-local build.
