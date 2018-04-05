#include <vector>
#include <algorithm>
#include <string>

#include <json/writer.h>

#include "PeakFinder.hh"
#include "ChannelData.hh"

// Turn in channel data to a json blob for sending to Redis
std::string daqAnalysis::ChannelData::Jsonify() {
  Json::FastWriter fastWriter;
  std::string str = fastWriter.write(GetJson());
  // erase newline
  str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
  return str;
}

std::string daqAnalysis::ChannelData::JsonifyPretty() {
  Json::StyledWriter writer;
  std::string str = writer.write(GetJson());
  return str;
}

Json::Value daqAnalysis::ChannelData::GetJson() {
  Json::Value output;
  output["baseline"] = baseline;
  output["max"] = max;
  output["min"] = min;
  output["rms"] = rms;
  output["channel_no"] = channel_no;
  output["empty"] = empty;
  
  output["last_channel_correlation"] = last_channel_correlation;
  output["last_channel_sum_rms"] = last_channel_sum_rms;
  output["next_channel_correlation"] = next_channel_correlation;
  output["next_channel_sum_rms"] = next_channel_sum_rms;

  output["peaks"] = Json::arrayValue;
  for (auto &peak: peaks) {
    Json::Value json_peak;
    json_peak["amplitude"] = peak.amplitude;
    json_peak["start_tight"] = peak.start_tight;
    json_peak["start_loose"] = peak.start_loose;
    json_peak["end_loose"] = peak.end_loose;
    json_peak["end_tight"] = peak.end_tight;
    output["peaks"].append(json_peak);
  }
  output["noise_ranges"] = Json::arrayValue;
  for (auto &range: noise_ranges) {
    Json::Value json_range = Json::arrayValue;
    json_range.append(range[0]);
    json_range.append(range[1]);
    output["noise_ranges"].append(json_range);
  } 

  return output;
}


