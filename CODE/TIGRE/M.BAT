@echo off
del tigre.lib
smartdrv c+
wmake -e -f tigre.mak  %1 > err
smartdrv c
type err
