#pragma once

#include <Python.h>  // has to be the first included header
#include <string>
#include <vector>

namespace StringUtility
{

//! extract from a line in a file the literal that follows after a string key and convert it to int
int getNumberAfterString(std::string line, std::string key);

//! remove beginning of line until (including) key, return removed substring
std::string extractUntil(std::string &line, std::string key);

//! remove whitespace (' ', '\t', '\n') at the beginning and end of the string
void trim(std::string &str);

//! output the values separated by spaces, after nValuesPerRow there will be a line break
void outputValuesBlock(std::ostream &stream, const std::vector<double> &values, int nValuesPerRow);

//! replace from by to
std::string replace(std::string str, const std::string& from, const std::string& to);

//! for N=1 output <str>, for N=2 output <str>*<str>, for N=3 output <str>*<str>*<str>
template<int N>
std::string multiply(std::string str);

};