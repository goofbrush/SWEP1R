# SWEP1R

## Reqs
uses qt for graphics, can be run without in console (see test.cpp)

## Compiling
### Compiling with qt:

(add mingw to path)
```
qmake
mingw32-make
```

### Compiling without qt:

```
g++ test.cpp -lPsapi
```
## Vscode tasks
Make sure qt is installed and mingw is added to path, then for propper vscode linter, either add the qt dir (eg C:\Qt\6.1.3) to your systems environment variables, or replace {QDIR} in [this json](.vscode/c_cpp_properties.json)
