plugins {
    alias(libs.plugins.android.application)
}

val cpmSourceCacheDir = System.getenv("HOME") + "/.cache/CPM/midicci"

android {
    namespace = "dev.atsushieno.midicci"
    compileSdk {
        version = release(libs.versions.androidTargetSdk.get().toInt()) {
            minorApiLevel = 1
        }
    }

    defaultConfig {
        applicationId = "dev.atsushieno.midicci"
        minSdk = libs.versions.androidMinSdk.get().toInt()
        targetSdk = libs.versions.androidTargetSdk.get().toInt()
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        externalNativeBuild {
            cmake {
                arguments.addAll(listOf(
                    "-DCMAKE_BUILD_TYPE=RelWithDebInfo",
                    "-DMIDICCI_SKIP_TOOLS=OFF",
                    "-DCPM_SOURCE_CACHE=$cpmSourceCacheDir",
                    "-DANDROID_STL=c++_shared"
                ))
                targets.add("main")
                cppFlags.add("-std=c++20")
                cppFlags.add("-fexceptions")
            }
        }
        ndk.abiFilters.addAll(listOf("arm64-v8a", "x86_64"))
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "4.1.2"
        }
    }
    buildFeatures {
        prefab = true
    }
}

dependencies {
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.appcompat)
    implementation(libs.material)
    implementation(files("../external/SDL3-3.4.0.aar"))
    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.junit)
    androidTestImplementation(libs.androidx.espresso.core)
}
