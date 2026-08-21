plugins {
    id("com.android.application")
}

android {
    namespace = "io.github.poptracker.android"
    compileSdk = 35
    ndkVersion = "27.3.13750724"

    defaultConfig {
        applicationId = "io.github.poptracker.android"
        minSdk = 21
        targetSdk = 35
        versionCode = 35_04_002
        versionName = "0.35.4-android.2"

        externalNativeBuild {
            cmake {
                arguments += listOf("-DANDROID_STL=c++_shared")
                abiFilters += listOf("arm64-v8a", "armeabi-v7a", "x86_64", "x86")
                cppFlags += listOf("-std=c++17")
            }
        }
    }

    buildFeatures {
        prefab = true
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    sourceSets {
        getByName("main") {
            java.srcDir("../vendor/SDL/android-project/app/src/main/java")
            assets.srcDir("../../assets")
        }
    }

    packaging {
        jniLibs {
            useLegacyPackaging = false
        }
        resources {
            excludes += setOf("META-INF/LICENSE*", "META-INF/NOTICE*")
        }
    }

    buildTypes {
        debug {
            isJniDebuggable = true
        }
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro",
            )
        }
    }
}

dependencies {
    implementation("androidx.core:core:1.16.0")
    implementation("io.github.ronickg:openssl:3.6.2-1")
}
