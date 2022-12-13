#include "analyzeDir.h"
#include <cassert>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <map>
#include <cstring>
#include <fstream>
#include <deque>
#include <algorithm>      // sort
#include <unordered_map>  // hist

using namespace std;


class directoryCheck{
  private:
    long numOfDirs = 0;
    long allFilesSize = 0;
    long largestFileSize = -1;
    
    string largestFilePath = "";
    string imageSize;
    vector<ImageInfo> largestImages;
    unordered_map<string,int> hist;

    // =========================== boolean functions for checks ============================== //
    //check if fullPath refers to a directory
    static bool isDir(const string &fullPath){
      struct stat buff;
      if (0 != stat(fullPath.c_str(), &buff)) return false;
      return S_ISDIR(buff.st_mode);
    }

    // check if fullPath refers to a file
    static bool isFile(const string &fullPath) {
      struct stat buff;
      if (0 != stat(fullPath.c_str(), &buff)) return false;
      return S_ISREG(buff.st_mode);
    }

    // check if string ends with a specific extension
    static bool endsWith(string & str, const string & suffix) {
      if (str.size() < suffix.size()) return false;
      else{
        return 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
      }
    }
  
    bool isImage(string fullPath){
      string cmd = "identify -format '%w %h' " + fullPath + " 2> /dev/null";
      
      auto fp = popen(cmd.c_str(), "r");
      assert(fp);
      char buff[PATH_MAX];
      if(fgets(buff, PATH_MAX, fp) != NULL) {
        imageSize = buff;
      }
      int status = pclose(fp);
      if(status != 0 or imageSize[0] == '0')
        imageSize = "";
      if(imageSize.empty())
        return false;
      else{
        return true;
      }
    }
  
    // =============================== other helper funcs ===================================== //
    static bool sortByPixels(const ImageInfo &img1, ImageInfo &img2){
      return (img1.width * img1.height > img2.width * img2.height);
    }
  
    // TO-DO: reference from tutorial
    deque<string> readTxtFile(string &fullPath){
      FILE * file = fopen(fullPath.c_str(), "r");
      char c;
      string current_word = "";
      deque<string> res;

      // contents of this while loop were referenced from "readingCfiles.cpp" 
      // found in Tutorials > Colin > Week4 > ReadingFilesCStyle
      while(fread(&c, sizeof(char), 1, file) != 0){
        c = tolower(c);
        if(!isalpha(c)){
          // a word will be at least 5 consecutive alpha characters
          if(current_word.length() >= 5){ 
            res.push_back(current_word);
          }
          current_word.clear();         // new word, clear it.
        }        
        else{
          // push character into the current word
          current_word.push_back(c);   
        }
      }
      if(current_word.length() >= 5){
        res.push_back(current_word);
      }
      fclose(file);
      return res;
    }

    // ============================== private getters ======================================== //

    pair<long, long> getWidthAndHeight(){
      string widthAndHeight = imageSize;  
      const char* delim = " ";
      vector<string> out;

      char *token = strtok(const_cast<char*>(widthAndHeight.c_str()), delim);
      while (token != nullptr) {
        out.push_back(string(token));
        token = strtok(nullptr, delim);
      }
      return make_pair(stol(out[0]), stol(out[1])); // width, height
    }
   // ======================================================================================= //

  public:
    // contents of this function were taken from lines 79-94 in word-histogram.cpp
    vector<pair<string, int>> getCommonWords(int &n){
      vector<pair<int, string>> arr;
      for(auto & h : hist){
        // we will have the num of occurence to be first (h.second) so that we can sort by int
        arr.emplace_back(-h.second, h.first);
      }
      if(arr.size() > size_t(n)) {
        // perform a partial sort since we need only N words 
        partial_sort(arr.begin(), arr.begin() + n, arr.end());
        arr.resize(n); // resize to keep the first N items
      }
      else {
        sort(arr.begin(), arr.end());
      }

      // swap to have vector<pair<string, int>>  
      vector<pair<string, int>> result;
      pair<string, int> swappedElements;
      for(auto & a : arr){
        swappedElements = make_pair(a.second.c_str(), -a.first);
        result.push_back(swappedElements);
      }
      return result;
    }
  
    // contents of this function were from referred to tutorial code:
    // - https://github.com/colinauyeung/CPSC457-F22-Notes/blob/master/Week5/dir_stat_complete/dir.cpp
    long getFilesize(string &filename) {
      struct stat s;
      // error handling: if stat() isn't successful, it won't return 0.
      // - return 0 as the size of the image
      if(stat(filename.c_str(), &s) != 0) {
        return 0;
      }
      return s.st_size;   
    }

    long getTotalFilesSize(){
      return allFilesSize;
    }

    vector<ImageInfo> getLargestImages(int &n){
      if(largestImages.size() > unsigned(n)){
        largestImages.resize(n);   
      }
      return largestImages;
    }

    long getNumOfDirs(){
      return numOfDirs;
    }

    pair<string, long> getLargestFile(){
      return make_pair(largestFilePath, largestFileSize);
    }

    // This recursive function was heavily inspired by the recursive function in analyzeDir.py
    pair<int, vector<string>> recurseDir(const string & dir){
      numOfDirs++;
      long numOfFiles = 0;

      vector<string> vacantDirs;
      DIR * dirPtr = opendir(dir.c_str());
      assert(dirPtr != nullptr);
      for (auto de = readdir(dirPtr); de != nullptr; de = readdir(dirPtr)) {
        string name = de->d_name;
        if (name == "." or name == "..") continue;
  
        string fullPath = dir + "/" + name;
        string path = fullPath;

        try{
          path = fullPath.substr(2);     // ignores "./"
        }
        catch(const std::exception& e){
          continue;
        }

        if (isFile(fullPath)){ 
          numOfFiles++;
          long fileSize = getFilesize(fullPath);

          allFilesSize += fileSize;
          if (fileSize > largestFileSize){
            largestFileSize = fileSize;
            largestFilePath = path;
          }

          // checking if it's an image
          if (isImage(fullPath)){
            pair<long, long> widthAndHeight = getWidthAndHeight();  // obtain the image width and height

            ImageInfo img;
            img.path = path;
            img.width = widthAndHeight.first;
            img.height = widthAndHeight.second;
            
            largestImages.push_back(img);
            sort(largestImages.begin(), largestImages.end(), sortByPixels);
          }

          // open txt file
          if (endsWith(name, ".txt")){
            deque<string> words = readTxtFile(fullPath); // returns deqeue of words
            for (string w: words){
              hist[w]++;
            }
          }
        }
        // checking if fullPath is a directory
        else if (isDir(fullPath)){
          pair<int, vector<string>> sub = recurseDir(fullPath);
          long subNumFiles = sub.first;
          numOfFiles = numOfFiles + subNumFiles;
          vector<string> subEmpties = sub.second;
          vacantDirs.insert(vacantDirs.end(), subEmpties.begin(), subEmpties.end());      // combine the vectors
        }
      }
      closedir(dirPtr);   // make sure to close directory
  
      if (numOfFiles == 0){
          vector<string> v;
          if (dir.size() > 1){
            v.push_back(dir.substr(2));   // ignore "./"
          }
          else{
            v.push_back(dir);
          }
          return make_pair(0, v);
      }
      else{
        return make_pair(numOfFiles, vacantDirs); 
      }
    }
};

// analyzeDir(n) computes stats about current directory
//   n = how many words and images to report in restuls
Results analyzeDir(int n){
  directoryCheck d = directoryCheck();
  string dirName = ".";

  pair<int, vector<string>> dirResults = d.recurseDir(dirName);

  Results res;
  res.n_files = dirResults.first;                      // total number of files in that directory. this is the first element returned from recurseDir()
  res.n_dirs = d.getNumOfDirs();                       // total num of directories 
  res.all_files_size = d.getTotalFilesSize();          // total size of all files in the directory

  pair<string, long> largestFile = d.getLargestFile();  // returns (path, file_size)
  res.largest_file_path = largestFile.first;            // path
  res.largest_file_size = largestFile.second;           // file size

  res.vacant_dirs = dirResults.second;                  // this is the first element returned from recurseDir()
  res.most_common_words = d.getCommonWords(n);          // we want only N common words from a text file
  res.largest_images = d.getLargestImages(n);           // we want only N largest images 

  return res;
}
