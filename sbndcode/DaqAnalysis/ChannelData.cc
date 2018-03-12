#include <vector>
#include <algorithm>
#include <string>

#include <json/writer.h>

#include "PeakFinder.hh"
#include "ChannelData.hh"

// Turn in channel data to a json blob for sending to Redis
std::string daqAnalysis::ChannelData::Jsonify() {
  Json::Value output;
  output["baseline"] = baseline;
  output["max"] = max;
  output["rms"] = rms;
  output["noise_sample"] = Json::arrayValue;
  for (auto &dat: noise_sample) {
    output["noise_sample"].append(dat);
  }
  output["peaks"] = Json::arrayValue;
  for (auto &peak: peaks) {
    Json::Value json_peak;
    json_peak["amplitude"] = peak.amplitude;
    json_peak["width"] = peak.width;
    json_peak["start"] = peak.start;
    json_peak["baseline"] = peak.baseline;
    output["peaks"].append(json_peak);
  }
  Json::FastWriter fastWriter;
  std::string str = fastWriter.write(output);
  // erase newline
  str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
  return str;
}

