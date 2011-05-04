#include "FileHandle.h"

FileHandle::FileHandle(int fid, const char* pid, int tl, int ml, const char* language){
	FileId = fid;
	TimeLimit = tl;
	MemoryLimit = ml;
	TimeUsed=0.0;
	MemoryUsed=0;
	ProblemId = pid;
	lang = language;
	result=true;
	sprintf(detailstatus,"\0");
}

int FileHandle::FetchFile(){
	if(FTPON) {
		int res = FileCurl.GetFileFromFTP(FileId);
		if(res==-1)	return -1;
	}
	else if(HTTPON){
		int res = FileCurl.GetFileFromHTTP(FileId);
		if(res==-1)	return -1;
	}
	else {
		strcpy(status, "IE");
		strcpy(detailstatus, "Cannot download source file. No method specified for downloading!!!");
		return -1;
	}
	return 0;
}

int FileHandle::CheckMIME(){
	FILE *fpipe;
    char line[256];
    sprintf(command, "file -b --mime-type %s%d.%s", FILEPATH, FileId, lang);
	
	if ( !(fpipe = (FILE*)popen(command,"r")) ){  
	// If fpipe is NULL
		perror("Problems with pipe");
		ToLogs("Problems with pipe");
		return -1;
	}
	else{
		if ( fgets( line, sizeof line, fpipe)){
			if(strncmp(line, "text", 4) != 0){
				result = false;
				strcpy(status, "CE");
				strcpy(detailstatus, "The source file is not a text file. Failed MIME check test.");
			}
			return 0;
		}
	}
	pclose(fpipe);
}

int FileHandle::MakeDir(){
	int ErrNo;
	char dirString[100];
	sprintf(dirString, "%s%d",FILEPATH, FileId);
	ToLogs("Creating directory with File Id");
	if( mkdir(dirString, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | S_IWOTH)==-1){
		ErrNo=errno;
		if(ErrNo==17) ToLogs("Directory already created.. Continuing");
		else {
			strcpy(status, "IE");
			sprintf(detailstatus, "Failure to create directory for file operations. Error Number: %d", errno);
			return -1;
		}
	}
	sprintf(systemString, "cp %s%d.txt %s%d/main.%s", FILEPATH, FileId, FILEPATH, FileId, lang);
	ToLogs(systemString);	
	if(system(systemString)==-1){
		ToLogs("Error in copying dowloaded file.");
		return -1;
	}
	return 0;
}

void FileHandle::Compile(){
	pipeCompile();
	if(CompileOutput.length()!=0){
		ToLogs("Compilation unsuccessful"); //ToLogs(CompileOutput.c_str());
		result = false;
		strcpy(status, "CE");
		strcpy(detailstatus, CompileOutput.c_str());
	}
	else ToLogs("Compilation successful\n");
}

void FileHandle::pipeCompile(){
	FILE *fpipe;
    if(strcmp(lang, "cpp")==0)
    	sprintf(command, "g++ -w -static %s%d/main.cpp -o %s%d/main 2>&1", FILEPATH, FileId, FILEPATH, FileId);
    else if(strcmp(lang, "c") == 0)
    	sprintf(command, "g++ -w -static %s%d/main.c -o %s%d/main 2>&1", FILEPATH, FileId, FILEPATH, FileId);
    else if(strcmp(lang, "java")==0)
		sprintf(command, "javac -nowarn -deprecation %s%d/main.java  2>&1", FILEPATH, FileId);    	
    ToLogs("Compiling file...");
	char line[256];
	
	if ( !(fpipe = (FILE*)popen(command,"r")) ){  
		perror("Problems with pipe");
		ToLogs("Problems with pipe");
	}
	else{
		while ( fgets( line, sizeof line, fpipe)){
			CompileOutput.append(line, strlen(line));
		}
	}
	pclose(fpipe);
}

int FileHandle::pipeNoOfTestCases(){
    FILE *fpipe;
    char line[256];
    sprintf(command, "ls %s%s/Input/ -l | egrep -c '^-'", TESTCASESPATH, ProblemId);
	
	if ( !(fpipe = (FILE*)popen(command,"r")) ){  
	// If fpipe is NULL
		perror("Problems with pipe");
		ToLogs("Problems with pipe");
		return -1;
	}
	else{
		while ( fgets( line, sizeof line, fpipe)){
			return atoi(line);
		}
	}
	pclose(fpipe);
}

int FileHandle::PrepareToExecute(){
	sprintf(systemString, "cp %s%s/Input/* %s%d/", TESTCASESPATH, ProblemId, FILEPATH, FileId);
	system(systemString);
	
	NoOfTestCases = pipeNoOfTestCases();
	if(NoOfTestCases==0){
		ToLogs("IE ERROR No Input File Specified. Please rectify the problem\n");
		strcpy(status, "IE");
		strcpy(detailstatus, "No Test Case file specified.");
		return -1;
	}
	sprintf(systemString, "Number of Test Cases = %d",NoOfTestCases);
	ToLogs(systemString);
	return 0;
}
	
void FileHandle::PipeExecute(){
	FILE *fpipe;
    if(strcmp(lang,"cpp")==0 || strcmp(lang,"c")==0){
    	sprintf(command, "./Execution %d %d %d %d %s", FileId, TestCaseId, TimeLimit, MemoryLimit, lang);
    }
    else if(strcmp(lang, "java")==0){
    	sprintf(command, "./java_Execution %d %d %d %d %s", FileId, TestCaseId, TimeLimit, MemoryLimit, lang);
    }
    
   	ToLogs(command);
	char line[1024];

	if ( !(fpipe = (FILE*)popen(command,"r")) ){  
	// If fpipe is NULL
		perror("Problems with pipe");
		ToLogs("Problems with pipe");
	}
	else{
		while ( fgets( line, sizeof line, fpipe)){
			ExecutionStr.append(line, strlen(line));
		}
	}
	pclose(fpipe);
}
	
void FileHandle::Execution(){
	for(TestCaseId=1; TestCaseId<=NoOfTestCases; TestCaseId++){
		PipeExecute();
		strcpy(str, ExecutionStr.c_str()); ToLogs(str);
		token = strtok(str, " \n");
		strcpy(status, token);
		if(strcmp(token, "AC")!=0) result=false;
		if(strcmp(token, "RE")==0 || strcmp(token, "IE")==0 ){
			token = strtok(NULL, "\n");
			strcpy(detailstatus, token);
		}
		token = strtok(NULL, " \n"); sprintf(tmp, "%s", token);
		TimeUsed+=(float)atof(tmp);
		if(TimeUsed>(float)TimeLimit){
			result = false;
			sprintf(status, "TLE");
			sprintf(detailstatus, "\0");
		}
		if(result==false) break;
	}
	
	for(TestCaseId=1; (result==true && TestCaseId<=NoOfTestCases); TestCaseId++){
		MatchOutput();
		if(result==false) strcpy(status,"WA");
	}
}
	
void FileHandle::pipeMatch(){
    FILE *fpipe;
    char line[256];
    
	if ( !(fpipe = (FILE*)popen(command,"r")) ){  
	// If fpipe is NULL
		perror("Problems with pipe");
		ToLogs("Problems with pipe");
	}
	else{
		while ( fgets( line, sizeof line, fpipe)){
			char tmp[90];
			sprintf(tmp, "%s", line);
			ToLogs(tmp);
			result = false;
		}
	}
	pclose(fpipe);
}

void FileHandle::SendResults(){
	sprintf(timeused, "%0.3f", TimeUsed);
	sprintf(memoryused, "%d", MemoryUsed);
	sprintf(fileid, "%d", FileId);
	sprintf(logs, "%s %s %s %s %s", fileid, status, detailstatus, timeused, memoryused); ToLogs(logs);
	FileCurl.SendResultsToWebpage(fileid, status, detailstatus, timeused, memoryused);
}
	
void FileHandle::MatchOutput(){
	char FromFileStr[100], ToFileStr[100];
	sprintf(FromFileStr, "%s%s/Output/%d.txt", TESTCASESPATH, ProblemId, TestCaseId);
	sprintf(ToFileStr, "%s%d/%do.txt", FILEPATH, FileId, TestCaseId);
	char cmd[100];
	sprintf(command, "diff %s %s --ignore-all-space --ignore-blank-lines --ignore-tab-expansion --ignore-space-change --brief 2>&1", FromFileStr, ToFileStr);
	pipeMatch();
}
	
void FileHandle::CleanUp(){
	sprintf(systemString, "rm -rf %s%d", FILEPATH, FileId);
	system(systemString);
}

void FileHandle::FileOperations(){

	if(FetchFile() == -1) return;
	if(CheckMIME() == -1) return;
	if(result==false) return;
	if(MakeDir()==-1) return;

	Compile();

	if(result==false){
		CleanUp();
		return;
	}

	if(PrepareToExecute()==-1){
		CleanUp();
		return;
	}

	Execution();

}

bool FileHandle::getResult(){
	return result;
}
void FileHandle::Action(){
	FileOperations();
	SendResults();
}

FileHandle::~FileHandle(){
	;//CleanUp();
}
