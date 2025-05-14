@echo off
echo MOVEFILE.BAT log > MOVEFILE.LOG
smartdrv c+ d+
rem After running MAKESTUF, run this to distribute files.

replace map.idx \bam\intdemo /U         >>MOVEFILE.LOG
replace map.idx \bam\intdemo /A         >>MOVEFILE.LOG
replace map.idx d:\bamdemo /U              >>MOVEFILE.LOG
replace map.idx d:\bamdemo /A              >>MOVEFILE.LOG
del map.idx                             >>MOVEFILE.LOG
replace *.map \bam\intdemo /U           >>MOVEFILE.LOG
replace *.map \bam\intdemo /A           >>MOVEFILE.LOG
replace *.map d:\bamdemo /U              >>MOVEFILE.LOG
replace *.map d:\bamdemo /A              >>MOVEFILE.LOG
del *.map                               >>MOVEFILE.LOG
replace main.stf \bam\intdemo /U        >>MOVEFILE.LOG
replace main.stf \bam\intdemo /A        >>MOVEFILE.LOG
replace main.stf d:\bamdemo /U      >>MOVEFILE.LOG
replace main.stf d:\bamdemo /A      >>MOVEFILE.LOG
del main.stf >>MOVEFILE.LOG

replace i:\bam\intdemo\code\src\*.* d:\code\src /U              >>MOVEFILE.LOG
replace i:\bam\intdemo\code\src\*.* d:\code\src /A              >>MOVEFILE.LOG
replace i:\bam\intdemo\code\tigre\*.* d:\code\tigre /U  >>MOVEFILE.LOG
replace i:\bam\intdemo\code\tigre\*.* d:\code\tigre /A  >>MOVEFILE.LOG

rem Place *.SMK and *.SCR in the STFS dir because they're loaded @ runtime.

cd ..
echo cd \bam\intdemo >> temp.bat
echo sweep attrib -a *.* >> temp.bat
echo cd \bam\intdemo\data >> temp.bat
echo smartdrv c d >> temp.bat
echo del temp.bat >> temp.bat
echo Process complete. >>MOVEFILE.LOG
temp
