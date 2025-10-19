# DMS Simulation
## Folder structure
`Distribution` contains files that will be copied to the target directory when the application is built.

`AnimationCatalog` contains [Animation Catalog](AnimationCatalog/Generated/AnimationCatalog.md).

`Documentation` contains [Design documentation](Documentation/Design/README.md).

`DMS_Simulation` contains UnrealEngine Project with all code and assets.

`LiveLink` contains a separate project for face motion capture using the `LiveLink Face App`.

The tests for the simulation toolchain have been moved to the [separate repository](https://devstack.vwgroup.com/bitbucket/projects/SDFL/repos/vom-virtual-occupant-scripts/).




## Setup instructions

### Hardware Requirements

**Note** these are requirements for **developing** and **building** the simulation toolchain. They are different in many respects from the those for **using** the toolchain to render videos and generate data. For the latter, please refer to the [User Manual](https://devstack.vwgroup.com/confluence/display/VOM/5.0+User+Manual#id-5.0UserManual-Minimalsystemrequirements).

* **CPU**: Quad-Core CPU or better
* **RAM**: 32 GB or more (16 GB _may_ work, but _could_ break during build).
* **Graphics Card**: a discrete graphics card with at least 4 GB RAM is recommended (GTX1660, Quadro series, RTX series). Most NVIDIA cards are known to work. With configuration tweaking, it is possible to open the project using integrated AMD graphics (Ryzen 4xxxG or newer). Intel integrated graphics do not work.
* **Disk**: at least 500 GB of disk space are needed if you want to build the executable. Solid State Drive is highly recommended. The git repo alone occupies 160 GB (80 for content, and 80 for LFS cache). On top of that, the shader cache is around 60 GB, intermediate files when building occupy 120GB. On top of that the actual build is around 30GB. Unreal Engine with debug symbols and Visual Studio occupy another 100 GB.

It recommended to have at least 100 GB free on your system drive (or the one where your swap file is located), as more memory may be required during cooking of content.

It is highly recommended to have a 2 TB SSD to be able to work comfortably.

### Software Requirements

* Win10 64 bit
* Unreal Engine 4.27
* Visual Studio 2019
* Git + Git LFS

Please see the below sections for information about licenses and setup instructions.


### Licenses
* To use Unreal Engine, you need to register at [Epic Games](https://www.unrealengine.com).

* For Visual Studio, you need to obtain a license for `Enterprise` or `Professional` edition from the IT Department.

If you don't have a license right now, the trial version can be used for 30 days.
Afterwards you won't be able to start Visual Studio, but the build tools will still continue to work (illegally).
Please note that companies with 100+ employees are not allowed to use the `Community` edition
with the exception of open-source projects and classroom use.


### Open SSH

It looks like windows 10 installs an openSSH client by default, however the package lacks the SSH agent. This can be fixed by installing OpenSSH server package in Windows and enabling the agent service as per document below.
https://docs.microsoft.com/en-us/windows-server/administration/openssh/openssh_keymanagement

Alternatively just go to Windows Settings -> Optional Features -> Add a feature -> Open SSH Client + Open SSH Server.

#### Generate SSH Keypair
Open command promt and type
```
ssh-keygen -t ed25519 -C "MyName@cariad.technology"
```
This will generate the files `C:\Users\username\.ssh\id_ed25519` and `C:\Users\username\.ssh\id_ed25519.pub`.


#### Enable SSH agent

**Note** this step is no longer necessary with recent versions of Windows and Git. Please skip it, and only come back if you have issues cloning the repo.


Open Admin Power Shell and paste the following.
```
# By default the ssh-agent service is disabled. Allow it to be manually started for the next step to work.
# Make sure you're running as an Administrator.
Get-Service ssh-agent | Set-Service -StartupType Manual

# Start the service
Start-Service ssh-agent

# This should return a status of Running
Get-Service ssh-agent

# Now load your key files into ssh-agent
ssh-add ~\.ssh\id_ed25519
```

### Git 
Download and install [Git](http://git-scm.com/). 

Please make sure to unselect (disable) the bundled "Git LFS", because it is not compatible with Cariad Bitbucket.

When installing Git select "Use external OPENSSH" when asked by the git installer.
Leave the remaining parameters to their default values.

### Git LFS
Download and install the git-lfs client. 
The newest version supported by Cariad bitbucket is https://github.com/git-lfs/git-lfs/releases/tag/v2.13.3 .

After installation right click in your work folder, select Git Bash Here, and type
```
git lfs install
```
This will update your local settings to benefit from Git LFS.

### Upload your public key to bitbucket
Open command promt
```
cd %userprofile%/.ssh
clip < id_ed25519.pub
```
Open your browser and go to https://devstack.vwgroup.com/bitbucket/plugins/servlet/ssh/account/keys (with your KUMS account).
Press add key and paste the clipboard contents. 
Confirm. You shall now have a new entry.


### Unreal Engine
Install Unreal Engine https://www.unrealengine.com/en-US/download/creators?install=true.

The project uses version 4.27.2 (sometimes offered as 4.27).

### Visual Studio
Install Visual Studio 2019. Visual Studio 2022 is not compatible.

https://my.visualstudio.com/Downloads?q=visual%20studio%202019&wt.mc_id=o~msft~vscom~older-downloads.

Login with your DXC account.

Download **Visual Studio Professional 2019 (version 16.9 or newer)**. Enterprise and Community would also work, so it is up to the license.

To add C++ tools to your VS installation, make sure you select **Game development with C++** and **.NET Desktop Development** under Workloads, as well as these additional options.
* C++ profiling tools
* C++ AddressSanitizer (optional)
* Windows 10 SDK (10.0.18362 or Newer) 


## Obtaining the source code
Clone this git repo. (Right click in your work folder, open git bash and type `git clone ssh://git@devstack.vwgroup.com:7999/sdfl/vom-virtual-occupant.git` ).

### Path to Unreal Engine

The build system assumes that Unreal Engine is installed to `C:\Program Files\Epic Games\UE_4.27`.

In case of a different location, set the environment variable `UEDIR` to the root of your Unreal Engine 4.27 installation, i.e. the `UE_4.27` folder. 

## Editing the project
In order to load the project into the Unreal Editor, you need to compile the plugins first.
Change to the repo folder and double click on `UnrealBuild.cmd`.
It will compile the libraries.
Now change to "DMS_Simulation" and click on `DMS_Simulation.uproject`. This will open Unreal Engine Project. Please note that depending on Performance of your computer, it can take significant time to compile the shaders.

When asked if you want to register extensions handler from Unreal Engine, select "Yes". This will enable the context menu you will need on the next step.



### Working on the C++ code

Right click on `DMS_Simulation.uproject` and select `Generate Visual Studio Project Files`. This will generate a file `DMS_Simulation.sln`, which can be opened with Visual Studio.

## Building the executable
### From the Unreal Editor
Load the project in Unreal Editor (see Editing the project). Select File -> Package Project -> Windows x64. Select the export folder and wait.
### From the command line
Open the command line in the repo folder and type `UnrealBuild.cmd Publish <Export Folder>`, where Export Folder is the location you want your executable to be packaged.

## Working with Git
For code changes, always create a branch.
It is advised to use Jira ticket number for branch name.
Either from GIT command line e.g. `git checkout -b DMSSIM-XXX`, or from Jira using the `Create Branch` link in the ticket.

Once the work is done and you are happy with the results (tests pass, etc.), add the new/changed files via `git add` and perform a commit via `git commit`.

For commit message use the pattern `DMSSIM-XXX: did this and that`.

Afterwards push the changes via `git push`.

**Note** if you are pushing a lot of LFS content, you may get the message `Connection timed out` while uploading, as a result the files will be uploaded, but the remote branch won't be updated.  You will notice it on the Git output message missing the remote branch information.
In this case, simply repeat the last push command. This time it will run faster and will update the remote branch.
