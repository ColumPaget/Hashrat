#!/bin/sh 

EXIT=0

Title()
{
echo -e "\n\033[01m ############### $* \033[00;39m"
}


OkayMessage()
{
echo -e "  \033[01;32mOKAY\033[00;39m    $1"
}

FailMessage()
{
echo -e "  \033[01;31mFAIL\033[00;39m    $1"
EXIT=3    # Bash says 1 and 2 are reserved for specific errors
}


TestHash()
{
RESULT=PASS
I=0

if [ "$4" = "" ]
then
	ITER=1
else
	ITER=$4
fi

if [ "$2" = "" ]
then
MSG="$1 Hashing"
else
MSG="$2"
fi

while [ "$I" -lt "$ITER" ]
do
	HR_OUT=`echo -n "The sky above the port was the color of television, tuned to a dead channel. " | ./hashrat -$1`
	if [ "$HR_OUT" != "$3" ]
	then
		RESULT=FAIL
	fi
	I=`expr $I + 1`
done


if [ "$RESULT" = "FAIL" ]
then
	FailMessage "$MSG BROKEN"
else
	OkayMessage "$MSG works"
fi
}


TestLocate()
{
HR_OUT=`echo $1 | ./hashrat -m -r .`

if [ "$HR_OUT" = "$2" ]
then
	OkayMessage "$3 works."
else 
	FailMessage "$3 BROKEN."
fi
}



##################### MAIN STARTS HERE ##########################

if [ ! -e ./hashrat ]
then
	FailMessage "'./hashrat' executable missing"
	echo
	exit 3
fi

Title "Testing Hash Types"
TestHash md5 "" 68e88e7b46a0fbd8a54c8932d2a9710d
TestHash sha1 "" d27f161a82d2834afccda6bfc1d10b2024fc6ec0
TestHash sha256 "" c7fadad016311a935a56dcdfb585cf5a4781073f7da13afa22177796e566434f
TestHash sha512 "" 0b8ac7af4b8e2dc449781888287aa50e9501b68766254b0c1bc6e17e7e86288c0a83b03d34f9c4c32836ca00a026323d8bbafc39f0c50f0c6b19200a28095595
TestHash whirlpool "" b690486285b18a9cbea3105a8f7e8ee439ef878530fe2e389e0b5ab17658df79ad6c83c1f836f81f51ce5c73a6899f0355fdad9f257526fc718ea04f7aa1b792
TestHash jh224 "" af0d674cdaaa7ec27b9c80acc763c6d51301c4273cd929fe043f67ca
TestHash jh256 "" 45dc6bfa4cd8bd55030ed3505b268f05431f16005dfcd775eb589f35e4ef6709
TestHash jh384 "" 55c63e4c22303227495c076ba0b11cda09a77856b98ee7d285283509415ca47141b09136daaada9fa3f10522456484db
TestHash jh512 "" 05feebb3148d9b0d12025759e4e054fe851dc6ad5bf58d3f79afb7d61caf8ce9983b7a0c6adab5dc2f186849ca0ea0236541ce4c659a6b4e1dd9748fc28eaf45

Title "Testing Repeated Iterations (may take some time)"
TestHash md5 "1000 md5" 68e88e7b46a0fbd8a54c8932d2a9710d 1000
TestHash sha1 "1000 sha1" d27f161a82d2834afccda6bfc1d10b2024fc6ec0 1000
TestHash whirlpool "1000 whirlpool" b690486285b18a9cbea3105a8f7e8ee439ef878530fe2e389e0b5ab17658df79ad6c83c1f836f81f51ce5c73a6899f0355fdad9f257526fc718ea04f7aa1b792 1000
TestHash jh384 "1000 jh384" 55c63e4c22303227495c076ba0b11cda09a77856b98ee7d285283509415ca47141b09136daaada9fa3f10522456484db 1000

Title "Testing Encoding"
TestHash 8 "base 8 (octal) encoding" 150350216173106240373330245114211062322251161015
TestHash 10 "base 10 (decimal) encoding" 104232142123070160251216165076137050210169113013
TestHash HEX "UPPERCASE base 16 (HEX) encoding" 68E88E7B46A0FBD8A54C8932D2A9710D
TestHash 64 "base 64 encoding" aOiOe0ag+9ilTIky0qlxDQ==
TestHash u64 "uu-encode style base 64 encoding" ":.B.>T:@^]BE3(DRTJEQ#0''"
TestHash x64 "xx-encode style base 64 encoding" "OCWCSoOUyxWZH6YmoeZl1E++"
TestHash p64 "'website compatible' base 64 encoding" "PDXDToPVyxX_I8Zmoe_l3F"
TestHash a85 "ASCII85 encoding" "B]tJ7\KYV+c_d]%3"
TestHash z85 "ZEROMQ85 encoding" "wX%ElWFTQ9+Z=X4h"


Title "Testing Misc. Features"

HR_OUT=`./hashrat -version`
if [ "$HR_OUT" = "version: 1.8.2" ]
then
	OkayMessage "Version (-version) works"
else
	FailMessage "Version (-version) BROKEN"
fi


HR_OUT=`./hashrat -sha1 -trad tests/quotes.txt`
if [ "$HR_OUT" = "5aa622e49b541f9a71409358d2e20feca1fa1f44  tests/quotes.txt" ]
then
	OkayMessage "File hashing works"
else
	FailMessage "File hashing BROKEN"
fi

HR_OUT=`./hashrat -dir -sha1 -trad tests`
if [ "$HR_OUT" = "b8b4058dc499ee1f330926a5a073a2c598b10c91  tests" ]
then
  OkayMessage "Directory hashing works"
else
  FailMessage "Directory hashing BROKEN"
fi


HR_OUT=`./hashrat -sha1 -trad -r tests | ./hashrat -sha1`
if [ "$HR_OUT" = "06af1d9f777bbeb1eecd76d71d869089683ded1b" ]
then
	OkayMessage "Recursive file hashing works"
else
	FailMessage "Recursive file hashing BROKEN"
fi

HR_INPUT="hash='sha1:5aa622e49b541f9a71409358d2e20feca1fa1f44' mode='100644' uid='0' gid='0' size='621' mtime='1423180289' inode='2359456' path='tests/quotes.txt'"
HR_OUT=`echo $HR_INPUT | ./hashrat -c 2>&1 | ./hashrat -sha256`

if [ "$HR_OUT" = "d50f1b64549232b7d79f094910ef3b578a65ffe5681a69907750e2519861dda7" ]
then
	OkayMessage "Checking files works"
else 
	FailMessage "Checking files BROKEN"
fi

HR_OUT=`./hashrat -r -dups tests`                      
if [ "$HR_OUT" = "DUPLICATE: tests/quotes.txt of tests/duplicate.txt " ]
then
	OkayMessage "Finding duplicate files works"
else 
	FailMessage "Finding duplicate files BROKEN"
fi


Title "Testing File Locate using different input formats"

TestLocate "hash='md5:6ec9de513a8ff1768eb4768236198cf3' mode='100644' uid='0' gid='0' size='621' mtime='1423180289' inode='2359456' path='test file'" "LOCATED: 6ec9de513a8ff1768eb4768236198cf3 'test file ' at ./tests/help.txt" "Locating files with native format input"
TestLocate "6ec9de513a8ff1768eb4768236198cf3  test file" "LOCATED: 6ec9de513a8ff1768eb4768236198cf3 'test file ' at ./tests/help.txt" "Locating files with traditional (md5sum) format input"
TestLocate "MD5 (test file) = 6ec9de513a8ff1768eb4768236198cf3" "LOCATED: 6ec9de513a8ff1768eb4768236198cf3 'test file ' at ./tests/help.txt" "Locating files with bsd format input"

HR_INPUT=`cat tests/test.ioc`
TestLocate "$HR_INPUT" "LOCATED: 6ec9de513a8ff1768eb4768236198cf3 ' Hashrat Test IOC' at ./tests/help.txt" "Locating files with OpenIOC input"


exit $EXIT
