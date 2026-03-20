# Quool Tool
### NOTE: The current oldest tested working windows version is windows 11
* Current target: windows 8

### Setup 
* clone the repo
* create a .env file in the root directory
* create a github fine grained token
* Put this inside the .env file and replace the `<YOUR FINE GRAINED TOKEN>` with your token:
```
{
    "github_api_key": "<YOUR FINE GRAINED TOKEN>"
}
```

### Building
* Run GenerateProjectFiles.bat
* open the VS solution
* Build/run from there

### SDL3 Static Linking
Note: The SDL_build_config.h file is replaced with one that I generated with cmake.
