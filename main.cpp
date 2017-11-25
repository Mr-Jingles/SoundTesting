

#include <vamp-hostsdk/PluginHostAdapter.h>
#include <vamp-hostsdk/PluginInputDomainAdapter.h>
#include <vamp-hostsdk/PluginLoader.h>

#include <iostream>
#include <fstream>
#include <set>
#include <sndfile.h>

#include <cstring>
#include <cstdlib>

#include "system.h"

#include <cmath>

using namespace std;

using Vamp::Plugin;
using Vamp::PluginHostAdapter;
using Vamp::RealTime;
using Vamp::HostExt::PluginLoader;
using Vamp::HostExt::PluginWrapper;
using Vamp::HostExt::PluginInputDomainAdapter;

enum Verbosity
{ //an enum type to specify output of plugin
    PluginIds,
    PluginOutputIds,
    PluginOutput,
    PluginInformation,
    PluginInformationDetailed
};

//example function to be delted later

string header(string text, int level)
{
    string out = '\n' + text + '\n';
    for (size_t i = 0; i < text.length(); ++i)
    {
        out += (level == 1 ? '=' : level == 2 ? '-' : '~');
    }
    out += '\n';
    return out;
}

//utility function converts type RealTime to a floating value in seconds

static double toSeconds(const RealTime &time)
{
    return time.sec + double(time.nsec + 1) / 1000000000.0;
}


//example function to modifiy it is called when runPlugin is finished
//needs to be modified to store results in a data structure

int printFeatures(int frame, int sr,
                  const Plugin::OutputDescriptor &output, int outputNo,
                  const Plugin::FeatureSet &features, ofstream *out, bool useFrames)
{
    static int featureCount = -1;

    if (features.find(outputNo) == features.end()) return 0;

    for (size_t i = 0; i < features.at(outputNo).size(); ++i)
    {

        const Plugin::Feature &f = features.at(outputNo).at(i);

        bool haveRt = false;
        RealTime rt;

        if (output.sampleType == Plugin::OutputDescriptor::VariableSampleRate)
        {
            rt = f.timestamp;
            haveRt = true;
        }
        else if (output.sampleType == Plugin::OutputDescriptor::FixedSampleRate)
        {
            int n = featureCount + 1;
            if (f.hasTimestamp)
            {
                n = int(round(toSeconds(f.timestamp) * output.sampleRate));
            }
            rt = RealTime::fromSeconds(double(n) / output.sampleRate);
            haveRt = true;
            featureCount = n;
        }

        if (useFrames)
        {

            int displayFrame = frame;

            if (haveRt)
            {
                displayFrame = RealTime::realTime2Frame(rt, sr);
            }

            (out ? *out : cout) << displayFrame;

            if (f.hasDuration)
            {
                displayFrame = RealTime::realTime2Frame(f.duration, sr);
                (out ? *out : cout) << "," << displayFrame;
            }

            (out ? *out : cout) << ":";

        }
        else
        {

            if (!haveRt)
            {
                rt = RealTime::frame2RealTime(frame, sr);
            }

            (out ? *out : cout) << rt.toString();

            if (f.hasDuration)
            {
                rt = f.duration;
                (out ? *out : cout) << "," << rt.toString();
            }

            (out ? *out : cout) << ":";
        }

        for (unsigned int j = 0; j < f.values.size(); ++j)
        {
            (out ? *out : cout) << " " << f.values[j];
        }
        (out ? *out : cout) << " " << f.label;

        (out ? *out : cout) << endl;
    }
}

//example function to be deleted later

void enumeratePlugins(Verbosity verbosity)
{
    PluginLoader *loader = PluginLoader::getInstance();

    if (verbosity == PluginInformation)
    {
        cout << "\nVamp plugin libraries found in search path:" << endl;
    }

    vector<PluginLoader::PluginKey> plugins = loader->listPlugins();
    typedef multimap<string, PluginLoader::PluginKey>
            LibraryMap;
    LibraryMap libraryMap;

    for (size_t i = 0; i < plugins.size(); ++i)
    {
        string path = loader->getLibraryPathForPlugin(plugins[i]);
        libraryMap.insert(LibraryMap::value_type(path, plugins[i]));
    }

    string prevPath = "";
    int index = 0;

    for (LibraryMap::iterator i = libraryMap.begin();
            i != libraryMap.end(); ++i)
    {

        string path = i->first;
        PluginLoader::PluginKey key = i->second;

        if (path != prevPath)
        {
            prevPath = path;
            index = 0;
            if (verbosity == PluginInformation)
            {
                cout << "\n  " << path << ":" << endl;
            }
            else if (verbosity == PluginInformationDetailed)
            {
                string::size_type ki = i->second.find(':');
                string text = "Library \"" + i->second.substr(0, ki) + "\"";
                cout << "\n" << header(text, 1);
            }
        }

        Plugin *plugin = loader->loadPlugin(key, 48000);
        if (plugin)
        {

            char c = char('A' + index);
            if (c > 'Z') c = char('a' + (index - 26));

            PluginLoader::PluginCategoryHierarchy category =
                    loader->getPluginCategory(key);
            string catstr;
            if (!category.empty())
            {
                for (size_t ci = 0; ci < category.size(); ++ci)
                {
                    if (ci > 0) catstr += " > ";
                    catstr += category[ci];
                }
            }

            if (verbosity == PluginInformation)
            {

                cout << "    [" << c << "] [v"
                        << plugin->getVampApiVersion() << "] "
                        << plugin->getName() << ", \""
                        << plugin->getIdentifier() << "\"" << " ["
                        << plugin->getMaker() << "]" << endl;

                if (catstr != "")
                {
                    cout << "       > " << catstr << endl;
                }

                if (plugin->getDescription() != "")
                {
                    cout << "        - " << plugin->getDescription() << endl;
                }

            }
            else if (verbosity == PluginInformationDetailed)
            {

                cout << header(plugin->getName(), 2);
                cout << " - Identifier:         "
                        << key << endl;
                cout << " - Plugin Version:     "
                        << plugin->getPluginVersion() << endl;
                cout << " - Vamp API Version:   "
                        << plugin->getVampApiVersion() << endl;
                cout << " - Maker:              \""
                        << plugin->getMaker() << "\"" << endl;
                cout << " - Copyright:          \""
                        << plugin->getCopyright() << "\"" << endl;
                cout << " - Description:        \""
                        << plugin->getDescription() << "\"" << endl;
                cout << " - Input Domain:       "
                        << (plugin->getInputDomain() == Vamp::Plugin::TimeDomain ?
                        "Time Domain" : "Frequency Domain") << endl;
                cout << " - Default Step Size:  "
                        << plugin->getPreferredStepSize() << endl;
                cout << " - Default Block Size: "
                        << plugin->getPreferredBlockSize() << endl;
                cout << " - Minimum Channels:   "
                        << plugin->getMinChannelCount() << endl;
                cout << " - Maximum Channels:   "
                        << plugin->getMaxChannelCount() << endl;

            }
            else if (verbosity == PluginIds)
            {
                cout << "vamp:" << key << endl;
            }

            Plugin::OutputList outputs =
                    plugin->getOutputDescriptors();

            if (verbosity == PluginInformationDetailed)
            {

                Plugin::ParameterList params = plugin->getParameterDescriptors();
                for (size_t j = 0; j < params.size(); ++j)
                {
                    Plugin::ParameterDescriptor & pd(params[j]);
                    cout << "\nParameter " << j + 1 << ": \"" << pd.name << "\"" << endl;
                    cout << " - Identifier:         " << pd.identifier << endl;
                    cout << " - Description:        \"" << pd.description << "\"" << endl;
                    if (pd.unit != "")
                    {
                        cout << " - Unit:               " << pd.unit << endl;
                    }
                    cout << " - Range:              ";
                    cout << pd.minValue << " -> " << pd.maxValue << endl;
                    cout << " - Default:            ";
                    cout << pd.defaultValue << endl;
                    if (pd.isQuantized)
                    {
                        cout << " - Quantize Step:      "
                                << pd.quantizeStep << endl;
                    }
                    if (!pd.valueNames.empty())
                    {
                        cout << " - Value Names:        ";
                        for (size_t k = 0; k < pd.valueNames.size(); ++k)
                        {
                            if (k > 0) cout << ", ";
                            cout << "\"" << pd.valueNames[k] << "\"";
                        }
                        cout << endl;
                    }
                }

                if (outputs.empty())
                {
                    cout << "\n** Note: This plugin reports no outputs!" << endl;
                }
                for (size_t j = 0; j < outputs.size(); ++j)
                {
                    Plugin::OutputDescriptor & od(outputs[j]);
                    cout << "\nOutput " << j + 1 << ": \"" << od.name << "\"" << endl;
                    cout << " - Identifier:         " << od.identifier << endl;
                    cout << " - Description:        \"" << od.description << "\"" << endl;
                    if (od.unit != "")
                    {
                        cout << " - Unit:               " << od.unit << endl;
                    }
                    if (od.hasFixedBinCount)
                    {
                        cout << " - Default Bin Count:  " << od.binCount << endl;
                    }
                    if (!od.binNames.empty())
                    {
                        bool have = false;
                        for (size_t k = 0; k < od.binNames.size(); ++k)
                        {
                            if (od.binNames[k] != "")
                            {
                                have = true;
                                break;
                            }
                        }
                        if (have)
                        {
                            cout << " - Bin Names:          ";
                            for (size_t k = 0; k < od.binNames.size(); ++k)
                            {
                                if (k > 0) cout << ", ";
                                cout << "\"" << od.binNames[k] << "\"";
                            }
                            cout << endl;
                        }
                    }
                    if (od.hasKnownExtents)
                    {
                        cout << " - Default Extents:    ";
                        cout << od.minValue << " -> " << od.maxValue << endl;
                    }
                    if (od.isQuantized)
                    {
                        cout << " - Quantize Step:      "
                                << od.quantizeStep << endl;
                    }
                    cout << " - Sample Type:        "
                            << (od.sampleType ==
                            Plugin::OutputDescriptor::OneSamplePerStep ?
                            "One Sample Per Step" :
                            od.sampleType ==
                            Plugin::OutputDescriptor::FixedSampleRate ?
                            "Fixed Sample Rate" :
                            "Variable Sample Rate") << endl;
                    if (od.sampleType !=
                            Plugin::OutputDescriptor::OneSamplePerStep)
                    {
                        cout << " - Default Rate:       "
                                << od.sampleRate << endl;
                    }
                    cout << " - Has Duration:       "
                            << (od.hasDuration ? "Yes" : "No") << endl;
                }
            }

            if (outputs.size() > 1 || verbosity == PluginOutputIds)
            {
                for (size_t j = 0; j < outputs.size(); ++j)
                {
                    if (verbosity == PluginInformation)
                    {
                        cout << "         (" << j << ") "
                                << outputs[j].name << ", \""
                                << outputs[j].identifier << "\"" << endl;
                        if (outputs[j].description != "")
                        {
                            cout << "             - "
                                    << outputs[j].description << endl;
                        }
                    }
                    else if (verbosity == PluginOutputIds)
                    {
                        cout << "vamp:" << key << ":" << outputs[j].identifier << endl;
                    }
                }
            }

            ++index;

            delete plugin;
        }
    }

    if (verbosity == PluginInformation ||
            verbosity == PluginInformationDetailed)
    {
        cout << endl;
    }
}


//function to modify that runs a plugin

int runPlugin(string programName, string pluginLibraryName, string id,
              string output, int outputNo, string wavname,
              string outfilename, bool useFrames)
{
    PluginLoader *loader = PluginLoader::getInstance();

    PluginLoader::PluginKey key = loader->composePluginKey(pluginLibraryName, id);

    SNDFILE *sndfile;
    SF_INFO sfinfo;
    memset(&sfinfo, 0, sizeof (SF_INFO));

    sndfile = sf_open(wavname.c_str(), SFM_READ, &sfinfo);
    if (!sndfile)
    {
        cerr << programName << ": ERROR: Failed to open input file \""
                << wavname << "\": " << sf_strerror(sndfile) << endl;
        return 1;
    }

    ofstream *out = 0;
    if (outfilename != "")
    {
        out = new ofstream(outfilename.c_str(), ios::out);
        if (!*out)
        {
            cerr << programName << ": ERROR: Failed to open output file \""
                    << outfilename << "\" for writing" << endl;
            delete out;
            return 1;
        }
    }

    Plugin *plugin = loader->loadPlugin
            (key, sfinfo.samplerate, PluginLoader::ADAPT_ALL_SAFE);
    if (!plugin)
    {
        cerr << programName << ": ERROR: Failed to load plugin \"" << id
                << "\" from library \"" << pluginLibraryName << "\"" << endl;
        sf_close(sndfile);
        if (out)
        {
            out->close();
            delete out;
        }
        return 1;
    }

    cerr << "Running plugin: \"" << plugin->getIdentifier() << "\"..." << endl;



    int blockSize = plugin->getPreferredBlockSize();
    int stepSize = plugin->getPreferredStepSize();

    if (blockSize == 0)
    {
        blockSize = 1024;
    }
    if (stepSize == 0)
    {
        if (plugin->getInputDomain() == Plugin::FrequencyDomain)
        {
            stepSize = blockSize / 2;
        }
        else
        {
            stepSize = blockSize;
        }
    }
    else if (stepSize > blockSize)
    {
        cerr << "WARNING: stepSize " << stepSize << " > blockSize " << blockSize << ", resetting blockSize to ";
        if (plugin->getInputDomain() == Plugin::FrequencyDomain)
        {
            blockSize = stepSize * 2;
        }
        else
        {
            blockSize = stepSize;
        }
        cerr << blockSize << endl;
    }
    int overlapSize = blockSize - stepSize;
    sf_count_t currentStep = 0;
    int finalStepsRemaining = max(1, (blockSize / stepSize) - 1); // at end of file, this many part-silent frames needed after we hit EOF

    int channels = sfinfo.channels;

    float *filebuf = new float[blockSize * channels];
    float **plugbuf = new float*[channels];
    for (int c = 0; c < channels; ++c) plugbuf[c] = new float[blockSize + 2];

    cerr << "Using block size = " << blockSize << ", step size = "
            << stepSize << endl;

    // The channel queries here are for informational purposes only --
    // a PluginChannelAdapter is being used automatically behind the
    // scenes, and it will take case of any channel mismatch

    int minch = plugin->getMinChannelCount();
    int maxch = plugin->getMaxChannelCount();
    cerr << "Plugin accepts " << minch << " -> " << maxch << " channel(s)" << endl;
    cerr << "Sound file has " << channels << " (will mix/augment if necessary)" << endl;

    Plugin::OutputList outputs = plugin->getOutputDescriptors();
    Plugin::OutputDescriptor od;
    Plugin::FeatureSet features;

    int returnValue = 1;
    int progress = 0;

    RealTime rt;
    PluginWrapper *wrapper = 0;
    RealTime adjustment = RealTime::zeroTime;

    if (outputs.empty())
    {
        cerr << "ERROR: Plugin has no outputs!" << endl;
        goto done;
    }

    if (outputNo < 0)
    {

        for (size_t oi = 0; oi < outputs.size(); ++oi)
        {
            if (outputs[oi].identifier == output)
            {
                outputNo = oi;
                break;
            }
        }

        if (outputNo < 0)
        {
            cerr << "ERROR: Non-existent output \"" << output << "\" requested" << endl;
            goto done;
        }

    }
    else
    {

        if (int(outputs.size()) <= outputNo)
        {
            cerr << "ERROR: Output " << outputNo << " requested, but plugin has only " << outputs.size() << " output(s)" << endl;
            goto done;
        }
    }

    od = outputs[outputNo];
    cerr << "Output is: \"" << od.identifier << "\"" << endl;

    if (!plugin->initialise(channels, stepSize, blockSize))
    {
        cerr << "ERROR: Plugin initialise (channels = " << channels
                << ", stepSize = " << stepSize << ", blockSize = "
                << blockSize << ") failed." << endl;
        goto done;
    }

    wrapper = dynamic_cast<PluginWrapper *> (plugin);
    if (wrapper)
    {
        // See documentation for
        // PluginInputDomainAdapter::getTimestampAdjustment
        PluginInputDomainAdapter *ida =
                wrapper->getWrapper<PluginInputDomainAdapter>();
        if (ida) adjustment = ida->getTimestampAdjustment();
    }

    // Here we iterate over the frames, avoiding asking the numframes in case it's streaming input.
    do
    {

        int count;

        if ((blockSize == stepSize) || (currentStep == 0))
        {
            // read a full fresh block
            if ((count = sf_readf_float(sndfile, filebuf, blockSize)) < 0)
            {
                cerr << "ERROR: sf_readf_float failed: " << sf_strerror(sndfile) << endl;
                break;
            }
            if (count != blockSize) --finalStepsRemaining;
        }
        else
        {
            //  otherwise shunt the existing data down and read the remainder.
            memmove(filebuf, filebuf + (stepSize * channels), overlapSize * channels * sizeof (float));
            if ((count = sf_readf_float(sndfile, filebuf + (overlapSize * channels), stepSize)) < 0)
            {
                cerr << "ERROR: sf_readf_float failed: " << sf_strerror(sndfile) << endl;
                break;
            }
            if (count != stepSize) --finalStepsRemaining;
            count += overlapSize;
        }

        for (int c = 0; c < channels; ++c)
        {
            int j = 0;
            while (j < count)
            {
                plugbuf[c][j] = filebuf[j * sfinfo.channels + c];
                ++j;
            }
            while (j < blockSize)
            {
                plugbuf[c][j] = 0.0f;
                ++j;
            }
        }

        rt = RealTime::frame2RealTime(currentStep * stepSize, sfinfo.samplerate);

        features = plugin->process(plugbuf, rt);

        printFeatures
                (RealTime::realTime2Frame(rt + adjustment, sfinfo.samplerate),
                 sfinfo.samplerate, od, outputNo, features, out, useFrames);

        if (sfinfo.frames > 0)
        {
            int pp = progress;
            progress = (int) ((float(currentStep * stepSize) / sfinfo.frames) * 100.f + 0.5f);
            if (progress != pp && out)
            {
                cerr << "\r" << progress << "%";
            }
        }

        ++currentStep;

    }
    while (finalStepsRemaining > 0);

    if (out) cerr << "\rDone" << endl;

    rt = RealTime::frame2RealTime(currentStep * stepSize, sfinfo.samplerate);

    features = plugin->getRemainingFeatures();

    printFeatures(RealTime::realTime2Frame(rt + adjustment, sfinfo.samplerate),
                  sfinfo.samplerate, od, outputNo, features, out, useFrames);

    returnValue = 0;

done:
    delete plugin;
    if (out)
    {
        out->close();
        delete out;
    }
    sf_close(sndfile);
    return returnValue;
}

int main(int argc, char** argv)
{
    enumeratePlugins(PluginInformationDetailed);

    string output = "";
    
    int value = runPlugin("VRConcert","Vamp-example-plugins","vamp-example-plugins:zerocrossing",
                          output,1,"song.wav",
                          "outputFile.txt",false);
    
    cout << output;



    return 0;
}

