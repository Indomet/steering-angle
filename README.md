# Table of contents
- [Introduction](#introduction)
- [Setup](#setup)
    - [Environment](#environment)
    - [Cloning and building the project](#cloning-the-project)
- [Team coordination](#team-coordination)

# Introduction


## Dependencies
* [Git](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git)
* [CMake](https://cmake.org/)


# Setup
## Environment
This project is based on linux, and is specifically made for Ubuntu-22.04 LTS.

## Cloning and building the project
* This quickstart guide is written with a linux shell in mind. If you use widnows or mac, consider running a virtual machine.
* Make sure you have git installed, otherwise refer to the [official documentation](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git)


Make sure your system is up to date and that CMake is installed
```
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install build-essential cmake git
```

In order to clone the project, open your terminal and paste the following ```git clone git@git.chalmers.se:courses/dit638/students/2024-group-02.git```
Afterwards, you will have the repository cloned. Change directories and create a build folder for cmake to build the project in


```
cd 2024-group-02/src
mkdir build
rm -rf ./*
```

Then build the project

```
cd build
cmake ..
make
make test
```
Afterwards you can run it with the following command
```
./helloworld <NUMBER>
```

# Team coordination
Each new feature will be introduced by creating an issue that describes the feature with a set of acceptance criteria. Each issue will be assigned to a member to work on where they create a branch that will close the issue upon resolving.

If any unprecedented behaviour is exhibited  by the software, a feature with a bug tag will be created and again, a corresponding branch will be created as well to fix the issue. The code will be verified to have fixed the issue before merging it.

Each merge request needs to be assigned to a team member, and a code review to be conducted before merging

Each commit message needs to be descriptive enough about what was changed followed by the issue number. For example ````git commit -m "added feature #2"```` 

# License
* This project is released under the terms of the GNU GPLv3 License