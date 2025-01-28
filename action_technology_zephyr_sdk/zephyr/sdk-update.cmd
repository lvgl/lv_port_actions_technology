@echo off
setlocal ENABLEDELAYEDEXPANSION

set PRJ_VER=%1
set PREFIX=%PRJ_VER:~0,3%
set VER_REF=
for /f %%a in ('git show-ref refs/heads/%PRJ_VER%') do (
    set VER_REF=%%a
)

if "%PRJ_VER%" == "" (
	@echo --== Usage: sdk-update [branch/tag] ==--
	goto:eof
)

@echo;
@echo --== git checkout %PRJ_VER% ==--
@echo;

call zephyr-env.cmd

git checkout .
git fetch

if "%PREFIX%" == "TAG" (
	if "%VER_REF%" == "" (
		git checkout -b %PRJ_VER% tags/%PRJ_VER%
	) else (
		git checkout %PRJ_VER%
	)
	west config manifest.file tag.yml
) else (
	if "%VER_REF%" == "" (
		git checkout -b %PRJ_VER% remotes/origin/%PRJ_VER%
	) else (
		git checkout %PRJ_VER%
		git rebase remotes/origin/%PRJ_VER%
	)
	west config manifest.file west.yml
)

call sdk-reset.cmd
