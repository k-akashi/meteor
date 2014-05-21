# METEOR Usage

# Require

* OS  
Ubuntu 12.04, 14.04  
Gentoo Linux  

* Packages:  
gcc, clang, libexpat

# Download, Compile 
\# git clone https://github.com/k-akashi/meteor.git  
\# cd meteor  
\# make  

# Example

* Scenario create  
Use deltaQ  
\# ./bin/deltaq scenario/scenario.xml

text to binary  
\# ./bin/scenario\_converter -i scenario/scenario.xml.out -o scenario/scenario.xml.bin

* Start meteor  
\# ./bin/meteor -Q scenario/scenario.xml.out -s scenario/scenario.xml.settings -d in -i 0 -I eth0 
