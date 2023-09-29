if [ "$#" -ne 1 ]; then
  echo "Usage: $0 <build_mode>" >&2
  exit 1
fi

mkdir build
if [ ! $? -eq 0 ]; then
	echo "Build folder does exist"
fi
cd build

CXX=g++
CC=gcc

export CXX
export CC

cmake .. -DCMAKE_BUILD_TYPE="$1" -DBUILD_STATIC=ON -DBUILD_SHARED=OFF -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF
if [ ! $? -eq 0 ]; then
	echo "Cmake was not successfull"
	exit 2
fi
gmake -j12
if [ ! $? -eq 0 ]; then
	echo "Could not fully build"
	exit 3
fi

cd ../
chmod +x dependency_builder.py
./dependency_builder.py --out=req_lib --in=bin/nomercy_game_module_x86.so
if [ ! $? -eq 0 ]; then
	echo "Could not copy dependencies"
	exit 3
fi

echo "build done!"