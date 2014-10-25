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


# NOTE
# It is assumed this will be run from the gemini root!

RSYNC=rsync
SOURCE=cb2:sdks
DESTINATION=build/dependencies
SDKS=(
	fbx_2015.1
	oculussdk_0.4.2
	oculussdk_0.4.3
)

# sync all sdks
for SDK in ${SDKS[@]}; do
	${RSYNC} --delete -az --exclude '*.DS_Store' --progress "${SOURCE}/${SDK}/" "${DESTINATION}/${SDK}"
done
