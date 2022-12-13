#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <string>
#include <vector>
#include <iostream>

using namespace std;

char buffer[1024*1024*100];
int buffSize = 0;
int buffPos = 0;

// Reads from STDIN_FILENO with a buffer size
// - body of this function was taken from: 
//     https://gitlab.com/cpsc457f22/longest-int-my-getchar/-/blob/main/fast-int.cpp
char readCharFromStdin()
{
  // check if buffer[] is empty, if it is replenish it using read()
  if(buffPos >= buffSize) {
    //    STDIN_FILENO has a fd as 0
    //    char* s = buffer
    //    streamsize = sizeof(buffer)
    buffSize = read(STDIN_FILENO, buffer, sizeof(buffer));
    // detect EOF
    if(buffSize <= 0) return -1;
    // reset position from where we'll return characters
    buffPos = 0;
  }
  // return the next character from the buffer and update position
  // maybe return a chunk of characters from the buffer instead individual char
  return buffer[buffPos++];
}

// Returns the entire text file as a string
string readFile(){
  string result;
  while(true) {
    char c = readCharFromStdin();

    // End of File, break loop
    if(c == -1) break;
    
    // Otherwise, append the character to the 'result' string
    result.push_back(c);
  }
  return result;
}

// this function is taken from is_palindrome() in slow-pali.cpp
bool isPalindrome(const string & s){
  for(size_t i = 0 ; i < s.size() / 2 ; i ++)
    if(tolower(s[i]) != tolower(s[s.size()-i-1]))
      return false;
  return true;
}

// contents of this function taken from get_longest_palindrome() in slow-pali.cpp
string getLongestPalindrome(){
  string maxPali;
  vector<string> words;

  while(1) {
    string stringFile = readFile();
    
    if(stringFile.size() == 0) break;

    // contents taken from the split function in slow-pali.cpp
    auto line = stringFile + " ";
    bool in_str = false;
    string curr_word = "";
    for(auto c : line) {
      if(isspace(c)) {
        if(in_str)
          words.push_back(curr_word);
        in_str = false;
        curr_word = "";
      }
      else {
        curr_word.push_back(c);
        in_str = true;
      }
    }

    for(auto word : words) {
      if(word.size() <= maxPali.size())
        continue;
      if(isPalindrome(word))
        maxPali = word;
    }
  }
  return maxPali;
}

// contents of this function taken from main() in slow-pali.cpp
int main()
{
  string maxPali = getLongestPalindrome();

  printf("Longest palindrome: %s\n", maxPali.c_str());
  return 0;
}
