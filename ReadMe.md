# SJR
Library for reading JSON files.

## Include

Include it as usual file.

```cpp
#include "SJR.h"
```
## Use

### Load
```cpp
#define SJR_IMPLEMENTATION
#include "SJR.h"


SJR json;
json.load("Filename.fileExtension");

```
### Read

If you have json file like the following :
```cpp
{
	"Weapon": "fangs",
	"LifePoints": 1000,
	"Speed": 3.14,
	"Enemy": false,
	"GoldPerItem": [1, 2, 3],
	"Ability":
	{
		"Attack": 20,
		"SpecialAttack": 40 
	}
}

```

you can get the values this way
```cpp
json["Weapon"].getValue<std::string>();				//"fangs"
json["LifePoints"].getValue<int>();				// 1000
json["Speed"].getValue<float>();				// 3.14
json["Enemy"].getValue<bool>();					// 0 (false)
json["GoldPerItem"][0].getValue<int>();				// 1
json["GoldPerItem"][1].getValue<int>();				// 2
json["GoldPerItem"][2].getValue<int>();				// 3
json["Ability"]["Attack"].getValue<int>();			// 20
json["Ability"]["SpecialAttack"].getValue<int>();		// 40
```


### Save

```cpp
json.save("FilenameWhereYouWantToSave.fileExtension");

```
