#include <vector>
#include <algorithm>
#include <string>

#include <json/writer.h>

#include "PeakFinder.hh"
#include "ChannelData.hh"

float daqAnalysis::ChannelData::meanPeakHeight() {
  if (peaks.size() == 0) {
    return 0;
  } 

  int total = 0;
  for (unsigned i = 0; i < peaks.size(); i++) {
    // account fot up/down peaks
    if (peaks[i].is_up) {
      total += peaks[i].amplitude - baseline;
    }
    else {
      total += baseline - peaks[i].amplitude;
    }
  }
  return ((float) total) / peaks.size();
}

// only count up peaks
float daqAnalysis::ChannelData::Occupancy() {
  float n_peaks = 0;
  for (unsigned i = 0; i < peaks.size(); i++) {
    if (peaks[i].is_up) {
      n_peaks += 1;
    }
  }
  return n_peaks;
}

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
  output["threshold"] = threshold;
  
  output["next_channel_dnoise"] = next_channel_dnoise;

  output["peaks"] = Json::arrayValue;
  for (auto &peak: peaks) {
    Json::Value json_peak;
    json_peak["amplitude"] = peak.amplitude;
    json_peak["start_tight"] = peak.start_tight;
    json_peak["start_loose"] = peak.start_loose;
    json_peak["end_loose"] = peak.end_loose;
    json_peak["end_tight"] = peak.end_tight;
    json_peak["is_up"] = peak.is_up;
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


