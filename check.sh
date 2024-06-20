#!/bin/bash 

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
		break
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


TestHMAC()
{
RESULT=PASS

if [ "$2" = "" ]
then
MSG="$1 HMAC"
else
MSG="$2"
fi

#this test is compatible with those on https://en.wikipedia.org/wiki/HMAC
HR_OUT=`echo -n "The quick brown fox jumps over the lazy dog" | ./hashrat -$1 -hmac key`

if [ "$HR_OUT" != "$3" ]
then
	RESULT=FAIL
fi


if [ "$RESULT" = "FAIL" ]
then
	FailMessage "$MSG BROKEN"
else
	OkayMessage "$MSG works"
fi
}




TestLocate()
{
HR_OUT=`echo $1 | ./hashrat -m -md5 -r .`

if [ "$HR_OUT" = "$2" ]
then
	OkayMessage "$3 works."
else 
	FailMessage "$3 BROKEN."
fi
}


TestLocateHook()
{
rm -f locatehook.out
HR_OUT=`echo $1 | ./hashrat -m -md5 -r . -hook "echo found > locatehook.out"`

if [ -e locatehook.out ]
then
	OkayMessage "$3 works."
else 
	FailMessage "$3 BROKEN."
fi
}



TestExitCodes()
{
case "$4" in 
FindDuplicates)
	HR_OUT=`./hashrat -r -dups $1`
	EXIT_FOUND=$?
	HR_OUT=`./hashrat -r -dups $2`
	EXIT_NOTFOUND=$?
;;

HashFile)
	HR_OUT=`./hashrat -md5 $2`
	EXIT_FOUND=$?
	HR_OUT=`./hashrat nonexistent 2>/dev/null`
	EXIT_NOTFOUND=$?
;;

*) 
	HR_OUT=`echo $1 $2 | ./hashrat -md5 $3 2>/dev/null`
	EXIT_FOUND=$?
	HR_OUT=`echo $1x $2 | ./hashrat -md5 $3 2>/dev/null`
	EXIT_NOTFOUND=$?
;;
esac

if [ "$EXIT_FOUND" = "0" -a "$EXIT_NOTFOUND" = "1" ]
then
	OkayMessage "$4 exit codes correct"
else 
	FailMessage "$4 exit codes BROKEN."
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
TestHash sha384 "" 74bad027d593889aecd64042cdad05d01792e8f36f2c65f19cbfce2e4a61ef72bccc1eea4188e59ed03d711daa1410f6
TestHash sha512 "" 0b8ac7af4b8e2dc449781888287aa50e9501b68766254b0c1bc6e17e7e86288c0a83b03d34f9c4c32836ca00a026323d8bbafc39f0c50f0c6b19200a28095595
TestHash whirlpool "" b690486285b18a9cbea3105a8f7e8ee439ef878530fe2e389e0b5ab17658df79ad6c83c1f836f81f51ce5c73a6899f0355fdad9f257526fc718ea04f7aa1b792
TestHash jh224 "" af0d674cdaaa7ec27b9c80acc763c6d51301c4273cd929fe043f67ca
TestHash jh256 "" 45dc6bfa4cd8bd55030ed3505b268f05431f16005dfcd775eb589f35e4ef6709
TestHash jh384 "" 55c63e4c22303227495c076ba0b11cda09a77856b98ee7d285283509415ca47141b09136daaada9fa3f10522456484db
TestHash jh512 "" 05feebb3148d9b0d12025759e4e054fe851dc6ad5bf58d3f79afb7d61caf8ce9983b7a0c6adab5dc2f186849ca0ea0236541ce4c659a6b4e1dd9748fc28eaf45

Title "Testing Repeated Iterations (may take some time)"
TestHash md5 "1000 md5" 68e88e7b46a0fbd8a54c8932d2a9710d 1000
TestHash sha1 "1000 sha1" d27f161a82d2834afccda6bfc1d10b2024fc6ec0 1000
TestHash sha256 "1000 sha256" c7fadad016311a935a56dcdfb585cf5a4781073f7da13afa22177796e566434f 1000
TestHash sha384 "1000 sha384" 74bad027d593889aecd64042cdad05d01792e8f36f2c65f19cbfce2e4a61ef72bccc1eea4188e59ed03d711daa1410f6 1000
TestHash whirlpool "1000 whirlpool" b690486285b18a9cbea3105a8f7e8ee439ef878530fe2e389e0b5ab17658df79ad6c83c1f836f81f51ce5c73a6899f0355fdad9f257526fc718ea04f7aa1b792 1000
TestHash jh384 "1000 jh384" 55c63e4c22303227495c076ba0b11cda09a77856b98ee7d285283509415ca47141b09136daaada9fa3f10522456484db 1000

Title "Testing HMAC digests"
TestHMAC md5 "" 80070713463e7749b90c2dc24911e275
TestHMAC sha1 "" de7c9b85b8b78aa6bc8a7a36f70a90701c9db4d9
TestHMAC sha256 "" f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8
TestHMAC sha384 "" d7f4727e2c0b39ae0f1e40cc96f60242d5b7801841cea6fc592c5d3e1ae50700582a96cf35e1e554995fe4e03381c237
TestHMAC sha512 "" b42af09057bac1e2d41708e48a902e09b5ff7f12ab428a4fe86653c73dd248fb82f948a549f7b791a5b41915ee4d1ec3935357e4e2317250d0372afa2ebeeb3a

Title "Testing Encoding"
TestHash 8 "base 8 (octal) encoding" 322177026032202322203112374315246277301321013040044374156300
TestHash 10 "base 10 (decimal) encoding" 210127022026130210131074252205166191193209011032036252110192
TestHash 16 "lowercase base 16 (hex) encoding" d27f161a82d2834afccda6bfc1d10b2024fc6ec0
TestHash HEX "UPPERCASE base 16 (HEX) encoding" D27F161A82D2834AFCCDA6BFC1D10B2024FC6EC0
TestHash 32 "base 32 encoding" "2J7RMGUC2KBUV7GNU274DUILEASPY3WA========"
TestHash 64 "base 64 encoding" "0n8WGoLSg0r8zaa/wdELICT8bsA="
TestHash x64 "xx-encode style base 64 encoding" "obwK4c9GUofwnOOzkR2960HwPg++"
TestHash p64 "'website compatible' base 64 encoding" "obwL6cAHVofwnPPzkS4A82IwQg0"
TestHash a85 "ASCII85 encoding" 'dXN#K$o9r6;+_9iW,lDP'
TestHash z85 "ZEROMQ85 encoding" "=SI2F3[n}kp9Zn?Ra>yK"


Title "Testing Misc. Features"

HR_OUT=`./hashrat -version`
if [ "$HR_OUT" = "version: 1.21" ]
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
	FailMessage "File hashing BROKEN [$HR_OUT]"
fi

HR_OUT=`./hashrat -dir -sha1 -trad tests`
if [ "$HR_OUT" = "0b858a85e778fe8b4ab386d185eb934978941e7b  tests" ]
then
  OkayMessage "Directory hashing works"
else
  FailMessage "Directory hashing BROKEN"
fi


HR_OUT=`./hashrat -sha1 -trad -r tests | ./hashrat -sha1`
if [ "$HR_OUT" = "7d99e0589d5fd237b108d9642c90c49bb0e58ef8" ]
then
	OkayMessage "Recursive file hashing works"
else
	FailMessage "Recursive file hashing BROKEN"
fi

HR_OUT=`./hashrat -trad -f tests/test.lst | ./hashrat -trad -md5`
if [ "$HR_OUT" = "d6ab85e877b3bef43975a168f22be006" ]
then
	OkayMessage "File hashing from a listfile works"
else
	FailMessage "File hashing from a listfile BROKEN"
fi


HR_OUT=`cat tests/test.lst | ./hashrat -trad -f - | ./hashrat -trad -md5`
if [ "$HR_OUT" = "d6ab85e877b3bef43975a168f22be006" ]
then
	OkayMessage "File hashing from a list on stdin works"
else
	FailMessage "File hashing from a list on stdin BROKEN"
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

TestLocate "hash='md5:6ec9de513a8ff1768eb4768236198cf3' mode='100644' uid='0' gid='0' size='621' mtime='1423180289' inode='2359456' path='test file'" "LOCATED: 6ec9de513a8ff1768eb4768236198cf3 'test file ' at tests/help.txt" "Locating files with native format input"
TestLocate "6ec9de513a8ff1768eb4768236198cf3  test file" "LOCATED: 6ec9de513a8ff1768eb4768236198cf3 'test file ' at tests/help.txt" "Locating files with traditional (md5sum) format input"
TestLocate "MD5 (test file) = 6ec9de513a8ff1768eb4768236198cf3" "LOCATED: 6ec9de513a8ff1768eb4768236198cf3 'test file ' at tests/help.txt" "Locating files with bsd format input"

HR_INPUT=`cat tests/test.ioc`
TestLocate "$HR_INPUT" "LOCATED: 6ec9de513a8ff1768eb4768236198cf3 ' Hashrat Test IOC' at tests/help.txt" "Locating files with OpenIOC input"

Title "Test hook functions"
TestLocateHook "hash='md5:6ec9de513a8ff1768eb4768236198cf3' mode='100644' uid='0' gid='0' size='621' mtime='1423180289' inode='2359456' path='test file'" "" "Hook function for file locate"
TestLocateHook "hash='md5:6933ee7eb504d29312b23a47d2dac374' mode='100644' uid='0' gid='0' size='621' mtime='1423180289' inode='2359456' path='test file'" "" "Hook function for file locate of files with bad characters in name"

Title "Testing exit codes for different operations"
TestExitCodes "6ec9de513a8ff1768eb4768236198cf3" "tests/help.txt" "" "HashFile"
TestExitCodes "tests" "libUseful-5" "-r -dups" "FindDuplicates"
TestExitCodes "6ec9de513a8ff1768eb4768236198cf3" "tests/help.txt" "-cf" "CheckHash"
TestExitCodes "6ec9de513a8ff1768eb4768236198cf3" "tests/help.txt" "-m -r ." "Locate"
TestExitCodes "tests" "libUseful-5" "-r -dups" "FindDuplicates"

echo
echo
if [ $EXIT -eq 0 ]
then
	OkayMessage "ALL CHECKS PASSED"
else
	FailMessage "SOME CHECKS FAILED"
fi

exit $EXIT
