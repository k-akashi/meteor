#!/bin/bash

# validate output of WiMAX computation module

NEW_OUTPUT="test_wimax_output.txt"
VALID_OUTPUT="test_wimax_output1-valid.txt"
SUCCESS_MESSAGE="\n\e[0;32mSUCCESS: Output of 'test_wimax' is valid.\e[00m\n"
FAILURE_MESSAGE="\n\e[1;31mVALIDATION FAILED: Output of 'test_wimax' differs from the valid one.\e[00m\n"
TROUBLE_MESSAGE="\n\e[1;35mERROR: 'diff' encountered troubles when processing the output of 'test_wimax'.\e[00m\n"

echo -e "\nSTART: Validating the output of 'test_wimax'...\n"

make && ./test_wimax > ${NEW_OUTPUT}
diff ${NEW_OUTPUT} ${VALID_OUTPUT}

DIFF_RESULT=$?

# check result of diff
if [ ${DIFF_RESULT} -eq 0 ]; then
    # no error => remove temporary file
    rm ${NEW_OUTPUT}
    echo -e ${SUCCESS_MESSAGE}
elif [ ${DIFF_RESULT} -eq 1 ]; then
    # error => failure
    echo -e ${FAILURE_MESSAGE}
else
    # unknown state => trouble 
    echo -e ${TROUBLE_MESSAGE}
    DIFF_RESULT=1
fi
