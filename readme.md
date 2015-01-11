#### about
This project is designed as a cross-platform game framework.
For more details, see the ideology section in the docs.


#### licensing
Unless otherwise noted, this code is provided under the BSD-2 clause
license. That means it's free for commercial and non-commercial use 
provided you retain the original copyright.

This project may contain code which is distributed under a different
license. See LICENSE for additional details.


#### foreword

Currently, there are private sdks which are not included in this repository.
This is by design to ensure most of this project's code can be open-sourced
while keeping private code and sdks off public repositories.

Therefore, this may NOT build out of the box without these SDKs.

If you still wish to build on your own, you would need to provide certain SDKs
yourself and place them in the right folder. These are:
	oculussdk
	openal-1.1

#### getting started

See the foreword first if you have not read it.

First, clone the repository.

```bash
git clone git://github.com/apetrone/gemini
```

Next, build the documentation (or peruse through docs/).

Running the following python script will setup the environment
and attempt to build the docs in formatted HTML. After this is
built, open "docs/html/index.html"

```python
python tools/bootstrap.py
```

See the documentation for more details on ideology, building, and code style.