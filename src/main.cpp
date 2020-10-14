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


#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

bool test_point(double lat, double lng) {
  if(lat >= 33.57 && lat <= 33.99 && lng >= -84.49 && lng <= -84.12) {
    return true;
  }
  return false;
}


std::vector<std::pair<std::string, std::vector<int>>> read_csv(std::string filename){
  // Reads a CSV file into a vector of <string, vector<int>> pairs where
  // each pair represents <column name, column values>
  // Create a vector of <string, int vector> pairs to store the result
  std::vector<std::pair<std::string, std::vector<int>>> result;
  // Create an input filestream
  std::ifstream myFile(filename);
  // Make sure the file is open
  if(!myFile.is_open()) throw std::runtime_error("Could not open file");
  // Helper vars
  std::string line, colname;
  int val;
  // Read the column names
  if(myFile.good()) {
    // Extract the first line in the file
    std::getline(myFile, line);
    // Create a stringstream from line
    std::stringstream ss(line);
    // Extract each column name
    while(std::getline(ss, colname, ',')) {
      // Initialize and add <colname, int vector> pairs to result
      result.push_back({colname, std::vector<int> {}});
    }
  }
  // Read data, line by line
  while(std::getline(myFile, line)) {
    // Create a stringstream of the current line
    std::stringstream ss(line);
    // Keep track of the current column index
    int colIdx = 0;
    // Extract each integer
    while(ss >> val) {
      // Add the current integer to the 'colIdx' column's values vector
      result.at(colIdx).second.push_back(val);
      // If the next token is a comma, ignore it and move on
      if(ss.peek() == ',') ss.ignore();
      // Increment the column index
      colIdx++;
    }
  }
  // Close file
  myFile.close();
  return result;
}


static std::string decompress(const std::string& filename) {
    using namespace std;  
    ifstream file(filename, ios_base::in | ios_base::binary);
    boost::filtering_streambuf<input> in;
    in.push(gzip_decompressor());
    in.push(file);
    boost::iostreams::copy(in, cout);
}


int extract_and_filter(std::string filename) {
  std::ifstream ifs(filename);
  std::string compressed_data( (std::istreambuf_iterator<char>(ifs) ),
      (std::istreambuf_iterator<char>()    ) );
  const char * compressed_pointer = compressed_data.data();
  std::string decompressed_data = gzip::decompress(compressed_pointer, 
      compressed_data.size());
}

int main(int argc, char **argv) {
  decompress(argv[1]);
}

/* Parse an octal number, ignoring leading and trailing nonsense. */
/*static int
  parseoct(const char *p, size_t n)
  {
  int i = 0;

  while ((*p < '0' || *p > '7') && n > 0) {
  ++p;
  --n;
  }
  while (*p >= '0' && *p <= '7' && n > 0) {
  i *= 8;
  i += *p - '0';
  ++p;
  --n;
  }
  return (i);
  }

  static int
  is_end_of_archive(const char *p)
  {
  int n;
  for (n = 511; n >= 0; --n)
  if (p[n] != '\0')
  return (0);
  return (1);
  }

  static void
  create_dir(char *pathname, int mode)
  {
  char *p;
  int r;

  if (pathname[strlen(pathname) - 1] == '/')
  pathname[strlen(pathname) - 1] = '\0';

  r = mkdir(pathname, mode);

  if (r != 0) {
  p = strrchr(pathname, '/');
  if (p != NULL) {
 *p = '\0';
 create_dir(pathname, 0755);
 *p = '/';
 r = mkdir(pathname, mode);
 }
 }
 if (r != 0)
 fprintf(stderr, "Could not create directory %s\n", pathname);
 }

 static FILE *
 create_file(char *pathname, int mode)
 {
 FILE *f;
 f = fopen(pathname, "wb+");
 if (f == NULL) {
 char *p = strrchr(pathname, '/');
 if (p != NULL) {
 *p = '\0';
 create_dir(pathname, 0755);
 *p = '/';
 f = fopen(pathname, "wb+");
 }
 }
 return (f);
 }

 static int
 verify_checksum(const char *p)
{
  int n, u = 0;
  for (n = 0; n < 512; ++n) {
    if (n < 148 || n > 155)
      u += ((unsigned char *)p)[n];
    else
      u += 0x20;

  }
  return (u == parseoct(p + 148, 8));
}

  static void
untar(FILE *a, const char *path)
{
  char buff[512];
  FILE *f = NULL;
  size_t bytes_read;
  int filesize;

  printf("Extracting from %s\n", path);
  for (;;) {
    bytes_read = fread(buff, 1, 512, a);
    if (bytes_read < 512) {
      fprintf(stderr,
          "Short read on %s: expected 512, got %d\n",
          path, (int)bytes_read);
      return;
    }
    if (is_end_of_archive(buff)) {
      printf("End of %s\n", path);
      return;
    }
    if (!verify_checksum(buff)) {
      fprintf(stderr, "Checksum failure\n");
      return;
    }
    filesize = parseoct(buff + 124, 12);
    switch (buff[156]) {
      case '1':
        printf(" Ignoring hardlink %s\n", buff);
        break;
      case '2':
        printf(" Ignoring symlink %s\n", buff);
        break;
      case '3':
        printf(" Ignoring character device %s\n", buff);
        break;
      case '4':
        printf(" Ignoring block device %s\n", buff);
        break;
      case '5':
        printf(" Extracting dir %s\n", buff);
        create_dir(buff, parseoct(buff + 100, 8));
        filesize = 0;
        break;
      case '6':
        printf(" Ignoring FIFO %s\n", buff);
        break;
      default:
        printf(" Extracting file %s\n", buff);
        f = create_file(buff, parseoct(buff + 100, 8));
        break;
    }
    while (filesize > 0) {
      bytes_read = fread(buff, 1, 512, a);
      if (bytes_read < 512) {
        fprintf(stderr,
            "Short read on %s: Expected 512, got %d\n",
            path, (int)bytes_read);
        return;
      }
      if (filesize < 512)
        bytes_read = filesize;
      if (f != NULL) {
        if (fwrite(buff, 1, bytes_read, f)
            != bytes_read)
        {
          fprintf(stderr, "Failed write\n");
          fclose(f);
          f = NULL;
        }
      }
      filesize -= bytes_read;
    }
    if (f != NULL) {
      fclose(f);
      f = NULL;
    }
  }
}

  int
main(int argc, char **argv)
{
  FILE *a;

  ++argv;
  for ( ;*argv != NULL; ++argv) {
    a = fopen(*argv, "rb");
    if (a == NULL)
      fprintf(stderr, "Unable to open %s\n", *argv);
    else {
      untar(a, *argv);
      fclose(a);
    }
  }
  return (0);
}*/
