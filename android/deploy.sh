#!/bin/bash

verify_success()
{
rc=$?;
if [[ $rc != 0 ]]; then
	exit $rc;
fi	
}

BUNDLE_IDENTIFIER=net.arcfusion.gemini

gradle build
verify_success


echo "Uninstalling old ${BUNDLE_IDENTIFIER}"
adb uninstall ${BUNDLE_IDENTIFIER}
verify_success

echo "Installing new ${BUNDLE_IDENTIFIER}..."
adb install build/outputs/apk/android-debug.apk
verify_success

echo "Wake the screen (may only work on Google devices)"
adb shell input keyevent 82
verify_success

echo "Start the activity..."
# start the activity
adb shell am start -n ${BUNDLE_IDENTIFIER}/${BUNDLE_IDENTIFIER}.gemini_activity
verify_success


# could also specify intent filter here
#adb shell am start -a android.intent.action.MAIN -n net.arcfusion.gemini/net.arcfusion.gemini.gemini_activity
# adb shell am start -n ${BUNDLE_IDENTIFIER}/android.app.NativeActivity