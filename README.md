# PythonCppExtension

## Build

To configure the build, CMake is required

### Using Python

```bash
python3 setup.py install
```

### Invoke CMake manually

```bash
cmake -B build
cmake --build build
```

## Usage

```python
	import pylex

	targets = ["ab+a", "u(f|g)?"]
	names = ["lexeme_1", "lexeme_2"]
	string = "abbba_ut_ug"

	p = pylex.Pattern(targets)
	for (target, str_offset, str_len) in pylex.finditer(p, string):
		print(f"found {names[target]}: {string[str_offset: str_offset + str_len]}");

	# output:
	# found lexeme_1: abbba
	# found lexeme_2: u
	# found lexeme_2: ug
```

