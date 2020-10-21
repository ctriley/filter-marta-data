#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <string>
#include <fstream>
#include <utility> // std::pair
#include <stdexcept> // std::runtime_error
#include <sstream> // std::stringstream
#include <chrono>
#include <regex>

#include <boost/filesystem.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

bool test_point(double lat, double lng) {
  if(lat >= 33.57 && lat <= 33.99 && lng >= -84.49 && lng <= -84.12) {
    return true;
  }
  return false;
}


std::vector<boost::filesystem::path> get_all(boost::filesystem::path const & root, std::string const & ext) {
  std::vector<boost::filesystem::path> paths;
  if (boost::filesystem::exists(root) && boost::filesystem::is_directory(root)) {
    for (auto const & entry : boost::filesystem::recursive_directory_iterator(root)) {
      if (boost::filesystem::is_regular_file(entry) && entry.path().extension() == ext)
        paths.emplace_back(entry.path());
    }
  }
  return paths;
}             


bool is_number( std::string token ) {
  return std::regex_match( token, 
      std::regex( ( "((\\+|-)?[[:digit:]]+)(\\.(([[:digit:]]+)?))?" ) ) );
}


std::string make_new_path(const std::string& filename) {
  std::string word = "xmode-standard-export-gold-usa-obfuscated";  
  std::string phrase = filename;
  for (int i = 0; i < phrase.length(); i++) {
    if (phrase[i] == ' ') {
      phrase[i + 1] = std::toupper(phrase[i + 1]);
    }
  }

  size_t pos = phrase.find(word); //find location of word
  phrase.erase(0,pos+word.size()); //delete everything prior to location found
  return phrase;
}


static int decompress(const std::string& filename, const std::string& outfolder) {
  auto new_path = make_new_path(filename);
  boost::filesystem::path output_path = boost::filesystem::absolute(outfolder) / new_path;
  std::string on = output_path.string();
  auto outfile_name = on.substr(0, on.size()-3);
  using namespace std; 
  stringstream ss;
  stringstream decompressed;
  std::string result;
  std::cout << outfile_name << std::endl;
  ifstream file(filename, ios_base::in | ios_base::binary);
  boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
  in.push(boost::iostreams::gzip_decompressor());
  in.push(file);
  boost::iostreams::copy(in, decompressed);
  int total_lines = 0;
  int included_lines = 0;
  for (std::string line; std::getline(decompressed, line); ){
    std::istringstream s(line);
    ++total_lines;
    std::string field;
    std::string lat, lng;
    getline(s, field, ',');
    getline(s, field, ',');
    getline(s, field, ',');
    getline(s, lat, ',');
    getline(s, lng, ',');
    if(is_number(lat) && is_number(lng)) { 
      if(test_point(stod(lat), stod(lng))) {
        ss << line;
        ++included_lines;
      }
    }
  }
  std::cout << "writing " << included_lines << " of " << total_lines << " lines." << std::endl;
  std::ofstream outfile;
  outfile.open(outfile_name);
  outfile << ss.rdbuf();
  return 0;
}

int main(int argc, char **argv) {
  auto outfolder = boost::filesystem::current_path() / "data";
  std::vector<boost::filesystem::path> paths = {boost::filesystem::absolute(argv[1])};
  if(boost::filesystem::is_directory(argv[1])) {
    paths = get_all(argv[1], ".gz");
  }
  int size = paths.size();
  int count = 1;
  for(auto p : paths) {
    auto start = std::chrono::high_resolution_clock::now();
    decompress(p.string(), outfolder.string());
    auto stop = std::chrono::high_resolution_clock::now();
    auto t = std::chrono::duration_cast<std::chrono::seconds>(stop - start); 
    std::cout << "decompressed " << std::to_string(count) << " of " << std::to_string(size) << " in " << t.count() << " seconds." << std::endl;
    ++count;
  }
}
