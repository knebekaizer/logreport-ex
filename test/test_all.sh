#!/usr/bin/env bash

echo "Generate customer DB..."
generators/gen_customers.py 10000 > data/cust_db.txt
echo "Generated `wc -l data/cust_db.txt` lines"

echo "Generate IP log..."
generators/gen_log.py 100000 > data/iplog.txt
echo "Generated `wc -l data/iplog.txt` lines"

echo "Run iplog..."
../_build/iplog data/cust_db.txt data/iplog.txt > data/report.txt
echo "Report created `wc -l data/report.txt` lines"

echo "Prepare validation. This may take a while..."
generators/report_simple.py data/cust_db.txt < data/iplog.txt > data/report_ok.txt

diff -q data/report.txt data/report_ok.txt && echo "Test passed" || echo "Test failed"
