
Assumptions
-----------

Below are some additional assumptions that have been made to overcome any
unclear or possibly inconsistent requirements:

0. _Customer ID is a string containing only ASCII characters in a range [A-Z]._ 
However, the example contains lower case letters as well as space and dash. 
    * Allowed characters are: [A-Za-z\-_]
    * Customer ID is case-insensitive
    
Testing
-------
There is a number of unit tests as well as built-in selftest.
 
Unit tests are based on Catch2 UT framework. Build target is iplog_ut

Self-test validates some invariants (aka "checked version"). 
Target iplog_SELFTEST produces normal report and additionally check invariants. 
Self-testing may badly affect overall performance.

Final test compares output data with "reference implementation". 
The reference is written in python using extremely simple (and slow) 
algorithm (sort of O(N^2) brute force).
This test is wrapped with test_all.sh script:
./test/test_all.sh 1000 10000
Parameters are: size of customer file and size of ip log. 