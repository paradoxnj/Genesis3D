REM setlocal

rem %1

REM set ssuser_old=%ssuser%
REM set ssdir_old=%SSDIR%

REM set ssuser=autobuild

REM set path=c:\winnt;c:\winnt\system32;c:\vss\win32;%path%
REM path

REM cd \
REM cd \dragon
REM md genesis
REM cd genesis

REM set SSDIR=%autobld%\vss\Avenger


ss Get $\Genesis10\tools\tpack -I- -GTM -R -Yautobuild,password

md msdev60
cd msdev60
ss Get $\Genesis10\msdev60 -I- -GTM -R -Yautobuild,password
cd ..

md bin
cd bin
ss Get $\bin -I- -GTM -R -Yautobuild,password
cd ..



REM set ssuser=%ssuser_old%
REM set SSDIR=%ssdir_old%

