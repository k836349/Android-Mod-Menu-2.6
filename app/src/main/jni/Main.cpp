#include <list>
#include <vector>
#include <string.h>
#include <pthread.h>
#include <cstring>
#include <jni.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include "Includes/Logger.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "KittyMemory/MemoryPatch.h"
#include "Menu.h"

#if defined(__aarch64__) // Compile for arm64 lib only
#include <And64InlineHook/And64InlineHook.hpp>
#else // Compile for armv7 lib only
#include <Substrate/SubstrateHook.h>
#include <Substrate/CydiaSubstrate.h>
#endif

// Fancy struct for patches for KittyMemory
struct My_Patches {
    MemoryPatch gold1, ruby1, godmode1, skill1;
} hexPatches;

bool feature1 = false, feature2 = false, featureHookToggle = false;
int sliderValue = 1;
void *instanceBtn;

// Function pointer for AddMoneyExample
void (*AddMoneyExample)(void *instance, int amount);

// Target lib here
#define targetLibName OBFUSCATE("libil2cpp.so")

// Hooking example
bool (*old_get_BoolExample)(void *instance);
bool get_BoolExample(void *instance) {
    if (instance != NULL && featureHookToggle) {
        return true;
    }
    return old_get_BoolExample(instance);
}

float (*old_get_FloatExample)(void *instance);
float get_FloatExample(void *instance) {
    if (instance != NULL && sliderValue > 1) {
        return (float) sliderValue;
    }
    return old_get_FloatExample(instance);
}

void (*old_Update)(void *instance);
void Update(void *instance) {
    instanceBtn = instance;
    old_Update(instance);
}

void *hack_thread(void *) {
    LOGI(OBFUSCATE("pthread created"));

    // Check if target lib is loaded
    do {
        sleep(1);
    } while (!isLibraryLoaded(targetLibName));

    LOGI(OBFUSCATE("%s has been loaded"), (const char *) targetLibName);

#if defined(__aarch64__) // Compile for arm64 lib only
    hexPatches.gold1 = MemoryPatch::createWithHex(targetLibName,
                                                 string2Offset(OBFUSCATE_KEY("0xDFE120", '-')),
                                                 OBFUSCATE("30 78 44 46 45 31 32 30"));
    hexPatches.ruby1 = MemoryPatch::createWithHex("libtargetLibHere.so",
                                                 string2Offset(OBFUSCATE_KEY("0xDFDB8C", '-')),
                                                 OBFUSCATE("30 78 44 46 44 42 38 43"));

    AddMoneyExample = (void(*)(void *,int))getAbsoluteAddress(targetLibName, 0x123456);
#else // Compile for armv7 lib only
    hexPatches.godmode1 = MemoryPatch::createWithHex(targetLibName,
                                                    string2Offset(OBFUSCATE_KEY("0x48", '-')),
                                                    OBFUSCATE("30 78 34 38"));
    hexPatches.skill1 = MemoryPatch::createWithHex("libtargetLibHere.so",
                                                  string2Offset(OBFUSCATE_KEY("0xC", '-')),
                                                  OBFUSCATE("30 78 43"));

    AddMoneyExample = (void (*)(void *, int)) getAbsoluteAddress(targetLibName, 0x123456);
#endif

    LOGI(OBFUSCATE("Done"));
    return NULL;
}

// JNI calls
extern "C" {
JNIEXPORT jobjectArray JNICALL
Java_uk_lgl_modmenu_FloatingModMenuService_getFeatureList(JNIEnv *env, jobject context) {
    jobjectArray ret;

    MakeToast(env, context, OBFUSCATE("Modded by khalid"), Toast::LENGTH_LONG);

    const char *features[] = {
            OBFUSCATE("Category_Features"),
            OBFUSCATE("1_Toggle_unlimited gold"),
            OBFUSCATE("2_Toggle_unlimited ruby"),
            OBFUSCATE("3_Toggle_god mode"),
            OBFUSCATE("4_Toggle_unlimited skills")
    };

    int Total_Feature = (sizeof features / sizeof features[0]);
    ret = (jobjectArray) env->NewObjectArray(Total_Feature, env->FindClass(OBFUSCATE("java/lang/String")),
                                             env->NewStringUTF(""));

    for (int i = 0; i < Total_Feature; i++)
        env->SetObjectArrayElement(ret, i, env->NewStringUTF(features[i]));

    return ret;
}

JNIEXPORT void JNICALL
Java_uk_lgl_modmenu_Preferences_Changes(JNIEnv *env, jclass clazz, jobject obj,
                                        jint featNum, jstring featName, jint value,
                                        jboolean boolean, jstring str) {
    const char *featureName = env->GetStringUTFChars(featName, 0);
    const char *TextInput = (str != NULL) ? env->GetStringUTFChars(str, 0) : "Empty";

    LOGD(OBFUSCATE("Feature name: %d - %s | Value: = %d | Bool: = %d | Text: = %s"), featNum,
         featureName, value, boolean, TextInput);

    switch (featNum) {
        case 1:
            feature1 = boolean;
            if (feature1) {
                hexPatches.gold1.Modify();
            } else {
                hexPatches.gold1.Restore();
            }
            break;
        case 2:
            feature2 = boolean;
            if (feature2) {
                hexPatches.ruby1.Modify();
            } else {
                hexPatches.ruby1.Restore();
            }
            break;
        case 3:
            if (boolean) {
                hexPatches.godmode1.Modify();
            } else {
                hexPatches.godmode1.Restore();
            }
            break;
        case 4:
            if (boolean) {
                hexPatches.skill1.Modify();
            } else {
                hexPatches.skill1.Restore();
            }
            break;
        default:
            break;
    }
}
}

__attribute__((constructor))
void lib_main() {
    pthread_t ptid;
    pthread_create(&ptid, NULL, hack_thread, NULL);
}
