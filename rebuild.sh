if [ "$#" -ne 1 ]; then
  echo "Usage: $0 <build_mode>" >&2
  exit 1
fi

rm -rf build
./build.sh $1