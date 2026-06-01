#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <WebViewFiles.h>

namespace
{
    std::vector<std::byte> streamToVector(juce::InputStream &stream)
    {
        using namespace juce;
        const auto sizeInBytes = static_cast<size_t>(stream.getTotalLength());
        std::vector<std::byte> result(sizeInBytes);
        stream.setPosition(0);
        [[maybe_unused]] const auto bytesRead =
            stream.read(result.data(), result.size());
        jassert(bytesRead == static_cast<ssize_t>(sizeInBytes));
        return result;
    }

    static const char *getMimeForExtension(const juce::String &extension)
    {
        static const std::unordered_map<juce::String, const char *> mimeMap = {
            {{"htm"}, "text/html"},
            {{"html"}, "text/html"},
            {{"txt"}, "text/plain"},
            {{"jpg"}, "image/jpeg"},
            {{"jpeg"}, "image/jpeg"},
            {{"svg"}, "image/svg+xml"},
            {{"ico"}, "image/vnd.microsoft.icon"},
            {{"json"}, "application/json"},
            {{"png"}, "image/png"},
            {{"css"}, "text/css"},
            {{"map"}, "application/json"},
            {{"js"}, "text/javascript"},
            {{"woff2"}, "font/woff2"}};

        if (const auto it = mimeMap.find(extension.toLowerCase());
            it != mimeMap.end())
            return it->second;

        jassertfalse;
        return "";
    }

    juce::Identifier getExampleEventId()
    {
        static const juce::Identifier id{"exampleEvent"};
        return id;
    }

// #ifndef ZIPPED_FILES_PREFIX
// #error \
//     "You must provide the prefix of zipped web UI files' paths, e.g., 'public/', in the ZIPPED_FILES_PREFIX compile definition"
// #endif

    /**
     * @brief Get a web UI file as bytes
     *
     * @param filepath path of the form "index.html", "js/index.js", etc.
     * @return std::vector<std::byte> with bytes of a read file or an empty vector
     * if the file is not contained in webview_files.zip
     */
    std::vector<std::byte> getWebViewFileAsBytes(const juce::String &filepath)
    {
        juce::MemoryInputStream zipStream{webview_files::webview_files_zip,
                                          webview_files::webview_files_zipSize,
                                          false};
        juce::ZipFile zipFile{zipStream};

        // 1. Zip 파일 내부의 실제 루트 경로(prefix)를 한 번만 계산해서 캐싱합니다.
        static juce::String basePrefix = "";
        static bool isPrefixDetermined = false;

        if (!isPrefixDetermined)
        {
            int minLength = std::numeric_limits<int>::max();
            for (int i = 0; i < zipFile.getNumEntries(); ++i)
            {
                if (const auto *entry = zipFile.getEntry(i))
                {
                    juce::String name = entry->filename;
                    
                    // 가장 경로가 짧은 index.html을 찾습니다. (dir/index.html 같은 중첩 파일 필터링)
                    if (name.endsWithIgnoreCase("index.html") && name.length() < minLength)
                    {
                        minLength = name.length();
                        // "index.html"의 길이(10)만큼 잘라내어 prefix만 남깁니다.
                        // 예: "../ui/dist/index.html" -> "../ui/dist/"
                        basePrefix = name.dropLastCharacters(10); 
                    }
                }
            }
            // Debug: print the determined prefix
            std::cout << "Determined base prefix for web resources: '" << basePrefix << "'" << std::endl;
            isPrefixDetermined = true;
        }

        // 2. 정확히 타겟팅된 전체 경로 생성
        juce::String targetPath = basePrefix + filepath;

        // 3. 해당 경로의 파일 추출
        if (auto *zipEntry = zipFile.getEntry(targetPath))
        {
            const std::unique_ptr<juce::InputStream> entryStream{
                zipFile.createStreamForEntry(*zipEntry)};

            if (entryStream == nullptr)
            {
                jassertfalse;
                return {};
            }

            return streamToVector(*entryStream);
        }

        std::cout << "Resource " << targetPath << " not found in zip" << std::endl;
        return {};
    }

    constexpr auto LOCAL_DEV_SERVER_ADDRESS = "http://localhost:5173";
} // namespace

//==============================================================================
// CONSTRUCTOR
WeatherSoundAudioProcessorEditor::WeatherSoundAudioProcessorEditor(WeatherSoundAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      webComponent(WebBrowserComponent::Options{}
                       .withBackend(WebBrowserComponent::Options::Backend::webview2)
                       .withWinWebView2Options(WebBrowserComponent::Options::WinWebView2{}
                                                   .withUserDataFolder(File::getSpecialLocation(File::SpecialLocationType::tempDirectory)))
                       .withResourceProvider([this](const auto &url)
                                             { return getResource(url); })
                       // .withInitialisationData("vendor", JUCE_COMPANY_NAME),
                       .withOptionsFrom(freqRelay)
                       .withOptionsFrom(resonanceRelay)

                       .withOptionsFrom(cloudCoverageRelay)
                       .withOptionsFrom(humidityRelay)
                       .withOptionsFrom(temperatureRelay)
                       .withOptionsFrom(uvIndexRelay)
                       .withOptionsFrom(windSpeedRelay)
                       .withOptionsFrom(windDirectionRelay)
                       .withOptionsFrom(visibilityRelay)
                       // Mix relays
                       .withOptionsFrom(cloudCoverageMixRelay)
                       .withOptionsFrom(humidityMixRelay)
                       .withOptionsFrom(temperatureMixRelay)
                       .withOptionsFrom(uvIndexMixRelay)
                       .withOptionsFrom(windSpeedMixRelay)
                       .withOptionsFrom(windDirectionMixRelay)
                       .withOptionsFrom(visibilityMixRelay)

                       .withOptionsFrom(bypassRelay)
                       .withNativeIntegrationEnabled() // Necessary
      )
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    addAndMakeVisible(webComponent);
#if JUCE_DEBUG
    // Debug mode: Load from local development server for hot-reloading
    webComponent.goToURL(LOCAL_DEV_SERVER_ADDRESS);
#else
    // Release mode: Load from bundled resources
    webComponent.goToURL(WebBrowserComponent::getResourceProviderRoot());
#endif

    // This is where our plugin’s editor size is set.
    setSize(640, 720);
}

// DECONSTRUCTOR
WeatherSoundAudioProcessorEditor::~WeatherSoundAudioProcessorEditor()
{
}

//==============================================================================
void WeatherSoundAudioProcessorEditor::paint(Graphics &g)
{
}

void WeatherSoundAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    webComponent.setBounds(0, 0, getWidth(), getHeight());

    // sets the position and size of the slider with arguments (x, y, width, height)
    // frequencySlider.setBounds (getWidth() / 2 - 50, getHeight() / 2 - 100, 100, 200);
    // frequencyLabel.setBounds(getWidth() / 2 - 50, getHeight() / 2 - 100, 100, 20);
    bypassButton.setBounds(getWidth() - 120, 30, 100, 30);
}

void WeatherSoundAudioProcessorEditor::sliderValueChanged(Slider *slider)
{
}

auto WeatherSoundAudioProcessorEditor::getResource(const juce::String &url) const
    -> std::optional<juce::WebBrowserComponent::Resource>
{
    std::cout << "ResourceProvider called with " << url << std::endl;

    const auto resourceToRetrieve =
        url == "/" ? "index.html" : url.fromFirstOccurrenceOf("/", false, false);

    if (resourceToRetrieve == "outputLevel.json")
    {
        juce::DynamicObject::Ptr levelData{new juce::DynamicObject{}};
        // levelData->setProperty("left", audioProcessor.outputLevelLeft.load());
        const auto jsonString = juce::JSON::toString(levelData.get());
        juce::MemoryInputStream stream{jsonString.getCharPointer(),
                                       jsonString.getNumBytesAsUTF8(), false};
        return juce::WebBrowserComponent::Resource{
            streamToVector(stream), juce::String{"application/json"}};
    }

    const auto resource = getWebViewFileAsBytes(resourceToRetrieve);
    if (!resource.empty())
    {
        const auto extension =
            resourceToRetrieve.fromLastOccurrenceOf(".", false, false);
        return juce::WebBrowserComponent::Resource{std::move(resource), getMimeForExtension(extension)};
    } else {
        std::cout << "Resource " << resourceToRetrieve << " not found in zip" << std::endl;
    }

    return std::nullopt;
}