#include "s3-sync.h"
#include "AWSManager.h"

#include <iostream>

using namespace std;

int main(int argc, char** argv)
{
	AWSManager manager("{ACCESSKEY}", "{PRIVATEKEY}", "eu-west-2");

	manager.ListBuckets();
	manager.put("C:\\Users\\v.majzus\\Desktop\\aws sync", "{BUCKET}");

	return 0;
}
