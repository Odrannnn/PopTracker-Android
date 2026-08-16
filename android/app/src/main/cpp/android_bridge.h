#pragma once

#include <string>

namespace AndroidBridge {

bool inputBox(
    const std::string& title,
    const std::string& message,
    const std::string& defaultValue,
    std::string& result,
    bool password
);

int messageBox(
    const std::string& title,
    const std::string& message,
    int buttons,
    int icon,
    int defaultResult
);

bool openDocument(const std::string& mimeType, std::string& path);
bool importTrackerPack();

// Consumes the most recent connection request delivered by an Android intent.
// This is read from PopTracker's SDL thread so native tracker state remains
// single-threaded even though onNewIntent runs on Android's UI thread.
bool takeLaunchRequest(
    std::string& host,
    std::string& slot,
    std::string& password,
    std::string& game
);

} // namespace AndroidBridge
