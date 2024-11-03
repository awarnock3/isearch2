@echo off
rem
rem Batch file to create the batch files to call the Isearch-cgi binaries
rem and to pass the correct parameters to them
rem
if "%1"=="" goto usage
if "%2"=="" goto usage

del isearch.cmd
echo @echo off > isearch.cmd
echo rem >> isearch.cmd
echo rem From this script, run the isrch_srch utility and pass a single argument >> isearch.cmd
echo rem that is the directory where your database are stored. >> isearch.cmd
echo rem >> isearch.cmd
echo rem For example: >> isearch.cmd
echo rem >> isearch.cmd
echo rem \path\to\Isearch-cgi\isrch_srch.exe \path\to\my\databases >> isearch.cmd
echo rem  >> isearch.cmd
echo %1\isrch_srch %2 >> isearch.cmd

del ifetch.cmd
echo @echo off > ifetch.cmd
echo rem  >> ifetch.cmd
echo rem From this script, run the isrch_fetch utility and pass 4 arguments: >> ifetch.cmd
echo rem  >> ifetch.cmd
echo rem \path\to\Isearch-cgi\isrch_fetch.exe databases_path %%1 %%2 %%3 >> ifetch.cmd
echo rem  >> ifetch.cmd
echo %1/isrch_fetch %2 %%1 %%2 %%3 >> ifetch.cmd

del ihtml.cmd
echo @echo off > ihtml.cmd
echo rem  >> ihtml.cmd
echo rem From this script, run the isrch_srch utility and pass a single argument >> ihtml.cmd
echo rem that is the directory where your database are stored. >> ihtml.cmd
echo rem  >> ihtml.cmd
echo rem For example: >> ihtml.cmd
echo rem  >> ihtml.cmd
echo rem \path\to\Isearch-cgi\isrch_html \path\to\my\databases >> ihtml.cmd
echo rem  >> ihtml.cmd
echo %1\isrch_html %2 >> ihtml.cmd

goto exit

:usage
  echo Usage:  Configure path-to-binaries path-to-databases

:exit