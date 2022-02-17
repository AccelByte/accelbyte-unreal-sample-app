# Justice Unreal SDK Plugin Development

For Justice Unreal SDK Plugin Developers only!

## One-time setup development environment

Setup for each Unreal Engine version e.g. 4.25, 4.26, 5.0EA

1. Run in PowerShell (Administrator):
```
.\setup-plugin-dev.ps1 -ue 4.25
.\setup-plugin-dev.ps1 -ue 4.26
.\setup-plugin-dev.ps1 -ue 5.0EA
```

2. Open Epic Game Installer and install each Unreal Engine version (see https://youtu.be/BASSlyAM7jg)

3. Create your own 'plugin-dev.config.ps1' based on LastPass Justice-Unreal-SDK-Plugin-Dev-Config (see 'plugin-dev.config.ps1.example' as a reference)

## Day-to-day development

Use 'plugin-dev.ps1' in PowerShell to run typical development tasks. 

1. Launch Visual Studio (default) or Rider
```
.\plugin-dev.ps1             # Default Unreal Engine version
.\plugin-dev.ps1 -ue 4.26    # Unreal Engine 4.26
```

2. Build Only (Win64 and Linux)
```
.\plugin-dev.ps1 -b Win64             # Default Unreal Engine version, Win64
.\plugin-dev.ps1 -ue 5.0EA -b Linux   # Unreal Engine 5.0EA, Linux
```

3. Run Tests Only (Win64) 
```
.\plugin-dev.ps1 -t AccelByte.Tests.A+AccelByte.Tests.B      Run test AccelByte.Tests.A and AccelByte.Tests.B against Justice backend 'dev' using default Unreal Engine version 
.\plugin-dev.ps1 -ue 5.0EA -e demo -t AccelByte.Tests.A      Run test AccelByte.Tests.A against Justice backend 'demo' using Unreal Engine 5.0EA 
```

4. See help for more details
```
.\plugin-dev.ps1 -h
```

ATTENTION: 

1. 'plugin-dev.ps1' helps you to set up a contained environment with all the necessary variables to perform development.

2. 'plugin-dev.ps1' does not permanently add or alter your system or user variables.

3. 'plugin-dev.ps1' will modify your Config\DefaultEngine.ini and Plugins\AccelByteUe4Sdk\AccelByteUe4Sdk.uplugin for local development only. DO NOT PUSH IT TO GIT REPOSITORY!

4. If you don't use 'plugin-dev.ps1', you need to setup all the necessary environment variables yourself.

5. Look inside 'plugin-dev.ps1' to see what environment variables that need to be setup.

## Debugging using Visual Studio (VS) and Unreal Editor (UE)

1. Make sure to launch Visual Studio using 'plugin-dev.ps1'

2. VS: Put a breakpoint in plugin source code if necessary

3. VS: Select "Development Editor" configuration and press F5 to launch Unreal Editor

4. UE: On menu bar, click "Window" -> "Test Automation"

5. UE: On Test Automation window, Tick the AccelByte tests to run and click "Start Tests"

## Development Quirks

* VS2019 Professional: Set limit to 10 GB only for reduced stuttering in Visual Studio https://stackoverflow.com/a/61601536
* VS2019 Professional: Fix unable to start debugger https://blog.notfaqs.com/2019/04/ue4-unable-to-start-debugger-in-visual.html
* Unreal Engine: Fix uproject file association https://forums.unrealengine.com/t/which-program-to-associate-a-uproject-with/13102/9
* Sourcetree: Use SSHClient OpenSSH to avoid the following error:
```
git -c filter.lfs.smudge= -c filter.lfs.required=false -c diff.mnemonicprefix=false -c core.quotepath=false --no-optional-locks clone --branch master --recursive git@bitbucket.org:accelbyte/justice-ue4-sdk-project.git D:\Git\justice-ue4-sdk-project
Cloning into 'D:\Git\justice-ue4-sdk-project'...
FATAL ERROR: Server unexpectedly closed network connection
fatal: the remote end hung up unexpectedly
fatal: early EOF
fatal: index-pack failed
```
