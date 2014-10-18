#!/bin/bash
# Adam Petrone
# October 2014
# 
# I'm creating a shell script to invoke rsync to copy private SDKs
# to the dependency folder. In most cases, I cannot re-distribute these
# sdks, even though they might be publicly available.
# 
# However, I still need a way to keep them updated and I haven't found
# an integrated git solution I like yet.

RSYNC=rsync
SOURCE=cb2:sdks
DESTINATION=build/dependencies
SDKS=(
	fbx_2015.1
	oculussdk_0.4.2
)

# navigate to gemini root
pushd ../../

# sync all sdks
for SDK in ${SDKS[@]}; do
	$RSYNC -az --progress "${SOURCE}/${SDK}/" "${DESTINATION}/${SDK}"
done

# restore original directory
popd

