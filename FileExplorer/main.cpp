#include <iostream>
#include <vector>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>
#include <pwd.h>
#include <grp.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <sys/ioctl.h>
#include <signal.h>
#include <fstream>
#include <string>
#include <bits/stdc++.h>

using namespace std;

enum cmdType
{
    cpy,
    mv,
    rname,
    create_file,
    create_dir,
    delete_file,
    delete_dir,
    gto,
    srch,
    quit,
    unknown
};

struct FileInfoStrct
{
    string Name;
    double Size;
    string UserName;
    string GroupName;
    char Permission[10];
    string ModTime;
    bool IsDirectory;
    mode_t Permission_ModeT;
};

void storePathTraversal(string);
void changeTerminalSettings();
void resetTerminalSettings();
cmdType getCommandType(string);
void modeToStr(mode_t, char*);
string GetRealPath(string);
string tostring(char*);

void EchoSignalChange();
string GetName();
string GetPath(string);
string StatusMsg;

void CopyFile(string, string);

vector<string> PathTraversal;
int PathTravsersalIndex = -1;
struct termios savedTerminalAttrs;
struct winsize TermSize;
string USER = getlogin();

class Directory
{
public:
    string DirPath = "";
    vector<string> FileNames;
    vector<struct FileInfoStrct> Files;
    int SelectedIndex = 0;
    int NumOfFiles = 0;

    Directory()
    {
    }

    void PopulateDirectoryStructure(string p_Path)
    {
        SelectedIndex = 0;
        NumOfFiles = 0;
        if (!FileNames.empty())
        {
            FileNames.clear();
            Files.clear();
            // free(Files);
        }
        DirPath = GetRealPath(p_Path);
        // cout << "After Clear" << endl;
        PopulateFileInfo();
    }

    void PopulateFileInfo()
    {
        DIR *dirStructure;
        struct dirent *file;
        dirStructure = opendir(DirPath.c_str());
        if (dirStructure != NULL)
        {
            for (file = readdir(dirStructure); file != NULL; file = readdir(dirStructure))
            {
                FileNames.emplace_back(file->d_name);
            }
        }
        // Sort the File Names in Ascending order
        if (!FileNames.empty())
        {
            sort(FileNames.begin(), FileNames.end());

            // struct FileInfo f[FileNames.size()];
            NumOfFiles = FileNames.size();
            Files.resize(NumOfFiles);

            for (int i = 0; i <= NumOfFiles - 1; i++)
            {
                struct stat fInfo;
                string FName = DirPath + "/" + FileNames[i];
                if (stat(FName.c_str(), &fInfo) == 0)
                {
                    // string a = FileNames[i];
                    Files[i].Name = FileNames[i];
                    Files[i].Size = fInfo.st_size / 1024.0;
                    // cout << "Size = " << Files[i].Size << endl;
                    modeToStr(fInfo.st_mode, Files[i].Permission);
                    Files[i].IsDirectory = (S_ISDIR(fInfo.st_mode)) ? true : false;

                    string t = ctime(&fInfo.st_mtim.tv_sec);
                    Files[i].ModTime = t.substr(0, t.length() - 1);

                    struct passwd *pwd;
                    struct group *grp;
                    pwd = getpwuid(fInfo.st_uid);
                    grp = getgrgid(fInfo.st_gid);
                    Files[i].UserName = pwd->pw_name;
                    Files[i].GroupName = grp->gr_name;

                    // time_t t = fInfo.st_mtim.tv_sec;
                }
            }
        }
    }

};

Directory dir;

void display(int rowLimitFrom, int rowLimitTo)
{
    string tmpDisplay;
    int us_len = USER.length();
    int sp = us_len - 4;
    if(sp < 0)
    {
        sp = sp*-1;
    }

    for (int i = rowLimitFrom; i <= rowLimitTo; i++)
    {
        tmpDisplay = ((dir.SelectedIndex == i) ? "  -->  " : "       ");
        tmpDisplay += dir.Files[i].Permission;
        //tmpDisplay += "        " + to_string(dir.Files[i].Size).substr(0, 8) + " KB" + "        " + dir.Files[i].UserName + "        " + dir.Files[i].GroupName + "        " + dir.Files[i].ModTime + "        " + dir.Files[i].Name;
        tmpDisplay += "        " + to_string(dir.Files[i].Size).substr(0, 8) + " KB" + "        ";
        tmpDisplay += dir.Files[i].UserName + "        ";
        if(dir.Files[i].UserName.length() < us_len)
        {
            for(int j=sp; j>0; j--)
            {
                tmpDisplay += " ";   
            }
        }
        tmpDisplay += dir.Files[i].GroupName + "        ";
        if(dir.Files[i].GroupName.length() < us_len)
        {
            for(int j=sp; j>0; j--)
            {
                tmpDisplay += " ";   
            }
        }

        tmpDisplay += dir.Files[i].ModTime + "        " + dir.Files[i].Name;
        // tmpDisplay += "\t" + to_string(dir.Files[i].Size) + "\t" + dir.Files[i].UserName + "\t" + dir.Files[i].GroupName + "\t" + dir.Files[i].ModTime + "\t" + dir.Files[i].Name;

        // tmpDisplay = ((SelectedIndex == i)? "  -->  " : "       ") + Files[i].Permission + "\t" + Files[i].Size + "\t" + Files[i].UserName << "\t" + Files[i].GroupName + "\t" + Files[i].ModTime + "\t" + Files[i].Name;

        if (TermSize.ws_col < tmpDisplay.length())
        {
            cout << tmpDisplay.substr(0, TermSize.ws_col - 1) << endl;
        }
        else
        {
            cout << tmpDisplay << endl;
        }

        // DisplayedRows++;;
    }

    // Fill rest of screen with Blank Lines
    while (rowLimitTo + 1 < TermSize.ws_row - 2)
    {
        cout << endl;
        rowLimitTo++;
    }
    cout << StatusMsg << endl;
}

// Moved the print function outside of class as SIGNAL-SIGWINCH also calls it and for including function in class requires to make it static
void PrintFilesInformation(int SigNum)
{
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &TermSize);
    // static int rowLimitFrom = 0, rowLimitTo = -1;
    static int rowLimitFrom = 0, rowLimitTo = TermSize.ws_row - 3;
    // static int DisplayedRows = -1;
    system("clear");

    if (SigNum == 1)
    {
        // For change of Directories, reset the variables.
        dir.SelectedIndex = 0;
        rowLimitFrom = 0;
        rowLimitTo = TermSize.ws_row - 3;
    }
    if (dir.NumOfFiles > 0)
    {
        if (dir.NumOfFiles > TermSize.ws_row - 2)
        {
            // Leaving Last before Line for Status\n and Last Line Empty (because of \n in Status Line. If not system.clear() is not working as expected)
            if (dir.SelectedIndex >= TermSize.ws_row - 2 && dir.SelectedIndex > rowLimitTo)
            {
                // DOWN arrow in overflow
                // rowLimitFrom = dir.SelectedIndex+1 - TermSize.ws_row-2;
                rowLimitFrom++;
                rowLimitTo = rowLimitFrom + TermSize.ws_row - 3;

                // Check if rowLimitTo is beyond the number of files to be displayed.
                rowLimitTo = (rowLimitTo > dir.NumOfFiles - 1) ? dir.NumOfFiles - 1 : rowLimitTo;
            }
            else if (dir.SelectedIndex > 0 && dir.SelectedIndex == rowLimitFrom)
            {
                // UP arrow in overflow
                rowLimitFrom--;
                rowLimitTo--;
            }
            else if (dir.SelectedIndex >= TermSize.ws_row - 2 && dir.SelectedIndex < rowLimitTo)
            {
                // Down ovberflow handled and using UP to scroll UP.
                // Do not update rowLimits
            }
            else if (dir.SelectedIndex > 0 && dir.SelectedIndex < rowLimitTo)
            {
                rowLimitFrom = 0;
                rowLimitTo = TermSize.ws_row - 3;
            }
            else
            {
                // Do  not update rowLimits
            }
        }
        else
        {
            rowLimitFrom = 0;
            rowLimitTo = dir.NumOfFiles - 1;
        }

        display(rowLimitFrom, rowLimitTo);
    }
}

void PrintFilesInformationResize(int SigNum)
{
    system("clear");
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &TermSize);
    int rowLimitFrom = 0, rowLimitTo = TermSize.ws_row - 3;
    rowLimitTo = (rowLimitTo > dir.NumOfFiles - 1) ? dir.NumOfFiles - 1 : rowLimitTo;
    dir.SelectedIndex = 0;
    display(rowLimitFrom, rowLimitTo);
}

class CommandHelper
{
public:
    string CommandType;
    //bool ContinueLoop = true;
    vector<string> InputPaths;

    void TokenizeInput(string InputStr)
    {
        char ch;
        int i=0, p=-1;
        ch = InputStr[i];
        bool parse = false;
        InputPaths.clear();

        while(ch != ' ' && i<=InputStr.length()-1)
        {
            //Get Command Type
            CommandType += ch;
            ch = InputStr[++i];
        }

        if(i < InputStr.length()-1)
        {
            ch = InputStr[i];
            while(i <= InputStr.length()-2)
            {
                if(ch == '"')
                {
                    parse = !parse;
                    if(parse == true)
                    {
                        InputPaths.emplace_back("");
                        p++;
                    }
                }
                else if(parse == true)
                {
                    InputPaths[p] += ch;
                }
                ch = InputStr[++i];
            }
        }
    }

    cmdType getCommandType()
    {
        cmdType cType;
        if (CommandType == "copy")
        {
            cType = cpy;
        }
        else if (CommandType == "move")
        {
            cType = mv;
        }
        else if (CommandType == "rename")
        {
            cType = rname;
        }
        else if (CommandType == "create_file")
        {
            cType = create_file;
        }
        else if (CommandType == "create_dir")
        {
            cType = create_dir;
        }
        else if (CommandType == "delete_file")
        {
            cType = delete_file;
        }
        else if (CommandType == "delete_dir")
        {
            cType = delete_dir;
        }
        else if (CommandType == "goto")
        {
            cType = gto;
        }
        else if (CommandType == "search")
        {
            cType = srch;
        }
        else if (CommandType == "quit")
        {
            cType = quit;
        }
        else
        {
            cType = unknown;
        }
        return cType;
    }

    void CreateFile(string path)
    {
        ofstream myfile;
        myfile.open(path);
        myfile.close();

        StatusMsg = "COMMAND MODE : File successfully Created";
    }

    void CreateFile(string path, mode_t ExistingMode)
    {
        ofstream myfile;
        myfile.open(path);
        myfile.close();

        chmod(path.c_str(), ExistingMode);

        StatusMsg = "COMMAND MODE : File successfully Created";
    }

    void CreateDir(string path, mode_t perm)
    {
        mkdir(path.c_str(), perm);

        StatusMsg = "COMMAND MODE : Directory successfully Created";
    }

    void DeleteFile(string path)
    {
        if (remove(path.c_str()) != 0)
        {
            StatusMsg = "COMMAND MODE : Error deleting file";
        }
        else
        {
            StatusMsg = "COMMAND MODE : File successfully deleted";
        }
    }

    void DeleteDir(string path)
    {
        vector<struct FileInfoStrct> ListOfFiles = GetFilesWithinDirectory(path);
        if(ListOfFiles.size() == 0)
        {
            return;
        }

        for(int i=0; i<=ListOfFiles.size()-1; i++)
        {
            if(ListOfFiles[i].IsDirectory == true)
            {
                DeleteDir(path + "/" + ListOfFiles[i].Name);
                DeleteFile(path + "/" + ListOfFiles[i].Name);
            }
            else
            {
                DeleteFile(path + "/" + ListOfFiles[i].Name);
            }
        }
        DeleteFile(path);

        StatusMsg = "COMMAND MODE : Directory successfully deleted";
    }

    void CopyFile(string sourcefile, string destinationfile)
    {
        ifstream fs;
        ofstream ft;
        string str;

        fs.open(sourcefile);
        ft.open(destinationfile);

        if (fs && ft)
        {
            while (getline(fs, str))
            {
                ft << str << "\n";
            }
        }
        else
        {
            StatusMsg = "COMMAND MODE : File Copy error";
        }

        fs.close();
        ft.close();
        StatusMsg = "COMMAND MODE : File(s) Copied";
    }

    void CopyDir(string source, string dest)
    {
        mode_t perm;
        vector<struct FileInfoStrct> ListOfFiles = GetFilesWithinDirectory(source);
        if(ListOfFiles.size() == 0)
        {
            return;
        }

        for(int i=0; i<=ListOfFiles.size()-1; i++)
        {
            if(ListOfFiles[i].IsDirectory == true)
            {
                CreateDir(dest + "/" + ListOfFiles[i].Name, ListOfFiles[i].Permission_ModeT);
                CopyDir(source + "/" + ListOfFiles[i].Name, dest + "/" + ListOfFiles[i].Name);
            }
            else
            {
                CreateFile(dest + "/" + ListOfFiles[i].Name, ListOfFiles[i].Permission_ModeT);
                CopyFile(source + "/" + ListOfFiles[i].Name, dest + "/" + ListOfFiles[i].Name);
            }
        }

        StatusMsg = "COMMAND MODE : Directory successfully copied";
    }

    bool Search(string path, string fname)
    {
        bool result = false;
        vector<struct FileInfoStrct> ListOfFiles = GetFilesWithinDirectory(path);
        if(ListOfFiles.size() == 0)
        {
            return false;
        }

        for(int i=0; i<=ListOfFiles.size()-1; i++)
        {
            if(ListOfFiles[i].Name == fname)
            {
                result = true;
                break;
            }
            if(ListOfFiles[i].IsDirectory == true)
            {
                result = Search(path + "/" + ListOfFiles[i].Name, fname);
                if(result == true)
                {
                    break;
                }
            }
        }

        return result;
    }

    void RenameFile(string OldName, string NewName)
    {
        if (rename(OldName.c_str(), NewName.c_str()) != 0)
        {
            StatusMsg = "COMMAND MODE : Error renaming file";
        }
        else
        {
            StatusMsg = "COMMAND MODE : File successfully renamed";
        }
    }

    vector<struct FileInfoStrct> GetFilesWithinDirectory(string path)
    {
        string present = ".";
        string prev = "..";
        vector<struct FileInfoStrct> Files;
        int i=0;

        DIR *dirStructure;
        struct dirent *file;
        dirStructure = opendir(path.c_str());

        if (dirStructure != NULL)
        {
            for (file = readdir(dirStructure); file != NULL; file = readdir(dirStructure))
            {
                struct stat fInfo;
                //if(file->d_name == "." || file->d_name == "..")
                if(strcmp(file->d_name, present.c_str()) == 0 || strcasecmp(file->d_name, prev.c_str()) == 0)
                {
                    continue;
                }
                string FName = path + "/" + file->d_name;
                if (stat(FName.c_str(), &fInfo) == 0)
                {
                    Files.emplace_back();
                    // string a = FileNames[i];
                    Files[i].Name = file->d_name;
                    //Files[i].Size = fInfo.st_size / 1024.0;
                    modeToStr(fInfo.st_mode, Files[i].Permission);
                    Files[i].Permission_ModeT = fInfo.st_mode;
                    Files[i].IsDirectory = (S_ISDIR(fInfo.st_mode)) ? true : false;

                    //string t = ctime(&fInfo.st_mtim.tv_sec);
                    //Files[i].ModTime = t.substr(0, t.length() - 1);


                    struct passwd *pwd;
                    struct group *grp;
                    pwd = getpwuid(fInfo.st_uid);
                    grp = getgrgid(fInfo.st_gid);
                    Files[i].UserName = pwd->pw_name;
                    Files[i].GroupName = grp->gr_name;

                    i++;
                    // time_t t = fInfo.st_mtim.tv_sec;
                }
            }
        }

        return Files;
    }

};


int main()
{
    string path = "/home/" + USER;
    char ch;
    bool loop = true;
    dir.PopulateDirectoryStructure(path);

    system("clear");
    StatusMsg = "NORMAL MODE : In Directory : " + dir.DirPath;
    PrintFilesInformation(1);
    PathTraversal.emplace_back(path);
    PathTravsersalIndex++;

    signal(SIGWINCH, PrintFilesInformationResize);

    changeTerminalSettings();
    while (loop)
    {
        ch = cin.get();
        switch (ch)
        {
        case 'A':
        {
            if (dir.SelectedIndex > 0)
            {
                dir.SelectedIndex--;
                StatusMsg = "NORMAL MODE : In Directory : " + dir.DirPath;
                PrintFilesInformation(0);
            }
            break;
        }

        case 'B':
        {
            if (dir.SelectedIndex < dir.NumOfFiles - 1)
            {
                dir.SelectedIndex++;
                StatusMsg = "NORMAL MODE : In Directory : " + dir.DirPath;
                PrintFilesInformation(0);
            }
            break;
        }

        // LEFT
        case 'D':
        {
            if (PathTravsersalIndex > 0)
            {
                PathTravsersalIndex--;
                path = PathTraversal[PathTravsersalIndex];

                dir.PopulateDirectoryStructure(path);
                StatusMsg = "NORMAL MODE : In Directory : " + dir.DirPath;
                PrintFilesInformation(1);
            }
            break;
        }

        // RIGHT
        case 'C':
        {
            if (PathTravsersalIndex < PathTraversal.size() - 1)
            {
                PathTravsersalIndex++;
                path = PathTraversal[PathTravsersalIndex];

                dir.PopulateDirectoryStructure(path);
                StatusMsg = "NORMAL MODE : In Directory : " + dir.DirPath;
                PrintFilesInformation(1);
            }
            break;
        }

        case 127:
        {
            path = dir.DirPath + "/..";
            dir.PopulateDirectoryStructure(path);
            StatusMsg = "NORMAL MODE : In Directory : " + dir.DirPath;
            PrintFilesInformation(1);

            storePathTraversal(path);

            break;
        }

        case 'h':
        {
            if (path == "/home/" + USER)
            {
                // If existing path is also HOME, then do not traverse.
                break;
            }
            path = "/home/" + USER;
            dir.PopulateDirectoryStructure(path);
            StatusMsg = "NORMAL MODE : In Directory : " + dir.DirPath;
            PrintFilesInformation(1);

            storePathTraversal(path);
            break;
        }

        case '\n':
        {
            int Sel = dir.SelectedIndex;
            if (dir.FileNames[Sel] == ".")
            {
                break;
            }
            if (dir.Files[Sel].IsDirectory == true)
            {
                path = dir.DirPath + "/" + dir.FileNames[Sel];
                dir.PopulateDirectoryStructure(path);
                StatusMsg = "NORMAL MODE : In Directory : " + dir.DirPath;
                PrintFilesInformation(1);

                storePathTraversal(path);
            }
            else
            {
                int pid;
                pid = fork();
                const char *xdgPath = "usr/bin/xdg-open";
                if (pid == 0)
                {
                    string tmp = dir.DirPath + "/" + dir.Files[Sel].Name;
                    execl("/usr/bin/xdg-open", "xdg-open", tmp.data(), (char *)0);
                    exit(1);
                }
                StatusMsg += "NORMAL MODE : Opened File : " + dir.Files[Sel].Name;
            }
            break;
        }

        case 'q':
        {
            loop = false;
            break;
        }

        // COMMAND Mode
        case ':':
        {
            CommandHelper C_Helper;

            EchoSignalChange();
            StatusMsg = "COMMAND MODE : ";
            PrintFilesInformation(1);

            bool commandMode = true;
            char ch;
            string inputStr;
            string source, dest;
            cmdType cmd_Type;
            

            while(commandMode == true)
            {
                C_Helper.CommandType = "";
                inputStr = "";
                source = "";
                dest = "";

                while (commandMode = true)
                {
                    ch = cin.get();
                    cout << ch;
                    if(ch == 27)
                    {
                        //Esc button
                        EchoSignalChange();
                        commandMode = false;
                        StatusMsg = "NORMAL MODE : In Directory : " + dir.DirPath;
                        break;
                    }
                    else if (ch == 127)
                    {
                        //Backspace
                        if(inputStr.length()>0)
                        {
                            inputStr = inputStr.substr(0, inputStr.length()-1);
                        }
                        PrintFilesInformation(1);
                        cout << inputStr;
                        continue;
                    }
                    else if (ch == '\n')
                    {
                        //Enter - Completion of input
                        break;
                    }
                    else
                    {
                        inputStr += ch;
                    }
                }

                if(commandMode == true)
                {
                    C_Helper.TokenizeInput(inputStr);
                    cmd_Type = C_Helper.getCommandType();

                    switch (cmd_Type)
                    {
                    case cpy:
                    {
                        if(C_Helper.InputPaths.size() > 1)
                        {
                            dest = C_Helper.InputPaths[C_Helper.InputPaths.size()-1];
                            dest = GetRealPath(dest);
                            for(int i=0; i<=C_Helper.InputPaths.size()-2; i++)
                            {
                                source = C_Helper.InputPaths[i];
                                if(source[0] != '/' && source[0] != '.' && source[0] != '~')
                                {
                                    source = dir.DirPath + "/" + source;
                                }
                                source = GetRealPath(source);
                                string fName = source.substr(source.find_last_of('/') + 1);
                                
                                struct stat fInfo;
                                if (stat(source.c_str(), &fInfo) != -1)
                                {
                                    if(S_ISDIR(fInfo.st_mode))
                                    {
                                        C_Helper.CreateDir(dest + "/" + fName, fInfo.st_mode);
                                        C_Helper.CopyDir(source, dest + "/" + fName);
                                    }
                                    else
                                    {
                                        C_Helper.CreateFile(dest + "/" + fName, fInfo.st_mode);
                                        C_Helper.CopyFile(source, dest + "/" + fName);
                                    }
                                }
                                else
                                {
                                    //Could not process the file info
                                    StatusMsg = "COMMAND MODE : Invalid Command";
                                }
                            }
                        }
                        else
                        {
                            //Invalid command
                            StatusMsg = "COMMAND MODE : Invalid Command";
                        }

                        break;

                        // // cout << "Inside Copy" << endl;
                        // source.clear();
                        // path = "";
                        // C_Helper.GetMultiFilesAndPath(source, path);
                        // if(source.size() == 0 || path == "")
                        // {
                        //     PrintFilesInformation(1);
                        //     continue;
                        // }
                        // // cout << "Fected Tokesn" << endl;
                        // for (int i = 0; i <= source.size() - 1; i++)
                        // {
                        //     // cout << "Source-" << source[i] << "Dest-" << path << endl;
                        //     struct stat fInfo;
                        //     string FName = path + "/" + source[i];
                        //     string ExistingFilePath = dir.DirPath + "/" + source[i];
                        //     if (stat(ExistingFilePath.c_str(), &fInfo) == 0)
                        //     {
                        //         C_Helper.CreateFile(path + "/" + source[i], fInfo.st_mode);
                        //         // cout << "File Created" << endl;
                        //         C_Helper.CopyFiles(source[i], path + "/" + source[i]);
                        //     }
                        // }

                        //break;
                    }
                    case mv:
                    {
                        if(C_Helper.InputPaths.size() > 1)
                        {
                            dest = C_Helper.InputPaths[C_Helper.InputPaths.size()-1];
                            dest = GetRealPath(dest);
                            for(int i=0; i<=C_Helper.InputPaths.size()-2; i++)
                            {
                                source = C_Helper.InputPaths[i];
                                if(source[0] != '/' && source[0] != '.' && source[0] != '~')
                                {
                                    source = dir.DirPath + "/" + source;
                                }
                                source = GetRealPath(source);
                                string fName = source.substr(source.find_last_of('/')+1);
                                
                                struct stat fInfo;
                                if (stat(source.c_str(), &fInfo) != -1)
                                {
                                    if(S_ISDIR(fInfo.st_mode))
                                    {
                                        C_Helper.CreateDir(dest + "/" + fName, fInfo.st_mode);
                                        C_Helper.CopyDir(source, dest + "/" + fName);
                                        C_Helper.DeleteDir(source);
                                        StatusMsg = "COMMAND MODE : Directory successfuly moved";
                                    }
                                    else
                                    {
                                        C_Helper.CreateFile(dest + "/" + fName, fInfo.st_mode);
                                        C_Helper.CopyFile(source, dest + "/" + fName);
                                        C_Helper.DeleteFile(source);
                                        StatusMsg = "COMMAND MODE : File(s) successfuly moved";
                                    }
                                }
                                else
                                {
                                    //Could not process the file info
                                    StatusMsg = "COMMAND MODE : Invalid Command";
                                }
                            }
                        }
                        else
                        {
                            //Invalid command
                            StatusMsg = "COMMAND MODE : Invalid Command";
                        }

                        break;


                        // source.clear();
                        // path = "";
                        // C_Helper.GetMultiFilesAndPath(source, path);
                        // if(source.size() == 0 || path == "")
                        // {
                        //     PrintFilesInformation(1);
                        //     continue;
                        // }
                        // for (int i = 0; i <= source.size() - 1; i++)
                        // {
                        //     C_Helper.CreateFile(path + "/" + source[i]);
                        //     // C_Helper.MoveFiles(source[i], path + "/" + source[i]);

                        //     struct stat fInfo;
                        //     string FName = path + "/" + source[i];
                        //     string ExistingFilePath = dir.DirPath + "/" + source[i];
                        //     if (stat(ExistingFilePath.c_str(), &fInfo) == 0)
                        //     {
                        //         C_Helper.CreateFile(path + "/" + source[i], fInfo.st_mode);
                        //         // cout << "File Created" << endl;
                        //         C_Helper.CopyFiles(source[i], path + "/" + source[i]);
                        //         C_Helper.DeleteFile(ExistingFilePath);
                        //     }
                        // }
                        // StatusMsg = "COMMAND MODE : File(s) successfuly moved";

                        // break;
                    }
                    case rname:
                    {
                        if(C_Helper.InputPaths.size() == 2)
                        {
                            source = C_Helper.InputPaths[0];
                            if(source[0] != '/' && source[0] != '.' && source[0] != '~')
                            {
                                source = dir.DirPath + "/" + source;
                            }
                            source = GetRealPath(source);

                            dest = C_Helper.InputPaths[1];
                            if(dest[0] != '/' && dest[0] != '.' && dest[0] != '~')
                            {
                                dest = dir.DirPath + "/" + dest;
                            }
                            dest = GetRealPath(dest);
                            C_Helper.RenameFile(source, dest);
                        }
                        else
                        {
                            StatusMsg = "COMMAND MODE : Invalid Command";
                        }

                        break;

                        // fName = C_Helper.GetName();
                        // if(fName == "")
                        // {
                        //     PrintFilesInformation(1);
                        //     continue;
                        // }
                        // string modFName = C_Helper.GetName();
                        // if(modFName == "")
                        // {
                        //     PrintFilesInformation(1);
                        //     continue;
                        // }
                        // C_Helper.RenameFile(fName, modFName);
                        // break;
                    }
                    case create_file:
                    {
                        if(C_Helper.InputPaths.size() == 2)
                        {
                            source = C_Helper.InputPaths[0];
                            dest = C_Helper.InputPaths[1];
                            dest = GetRealPath(dest);

                            C_Helper.CreateFile(dest + "/" + source);
                        }
                        else
                        {
                            StatusMsg = "COMMAND MODE : Invalid Command";
                        }

                        break;

                        
                        // fName = C_Helper.GetName();
                        // if(fName == "")
                        // {
                        //     PrintFilesInformation(1);
                        //     continue;
                        // }
                        // path = C_Helper.GetPath();
                        // if(path == "")
                        // {
                        //     PrintFilesInformation(1);
                        //     continue;
                        // }
                        // path = C_Helper.GetRealPath(path);
                        // path += "/" + fName;
                        // C_Helper.CreateFile(path);
                        break;
                    }
                    case create_dir:
                    {
                        if(C_Helper.InputPaths.size() == 2)
                        {
                            source = C_Helper.InputPaths[0];
                            dest = C_Helper.InputPaths[1];
                            dest = GetRealPath(dest);

                            C_Helper.CreateDir(dest + "/" + source, 0777);
                        }
                        else
                        {
                            StatusMsg = "COMMAND MODE : Invalid Command";
                        }

                        break;


                        // fName = C_Helper.GetName();
                        // if(fName == "")
                        // {
                        //     PrintFilesInformation(1);
                        //     continue;
                        // }
                        // path = C_Helper.GetPath();
                        // if(path == "")
                        // {
                        //     PrintFilesInformation(1);
                        //     continue;
                        // }

                        // path += "/" + fName;
                        // C_Helper.CreateDir(path, perm);
                        // break;
                    }
                    case delete_file:
                    {
                        if(C_Helper.InputPaths.size() == 1)
                        {
                            source = C_Helper.InputPaths[0];
                            if(source[0] != '/' && source[0] != '.' && source[0] != '~')
                            {
                                source = dir.DirPath + "/" + source;
                            }
                            source = GetRealPath(source);

                            C_Helper.DeleteFile(source);
                        }
                        else
                        {
                            StatusMsg = "COMMAND MODE : Invalid Command";
                        }

                        break;


                        // path = C_Helper.GetPath();
                        // if(path == "")
                        // {
                        //     PrintFilesInformation(1);
                        //     continue;
                        // }
                        // C_Helper.DeleteFile(path);
                        // break;
                    }
                    case delete_dir:
                    {
                        if(C_Helper.InputPaths.size() == 1)
                        {
                            source = C_Helper.InputPaths[0];
                            if(source[0] != '/' && source[0] != '.' && source[0] != '~')
                            {
                                source = dir.DirPath + "/" + source;
                            }
                            source = GetRealPath(source);

                            C_Helper.DeleteDir(source);
                        }
                        else
                        {
                            StatusMsg = "COMMAND MODE : Invalid Command";
                        }

                        break;


                        // path = C_Helper.GetPath();
                        // if(path == "")
                        // {
                        //     PrintFilesInformation(1);
                        //     continue;
                        // }

                        // path = C_Helper.GetRealPath(path);
                        // C_Helper.DeleteDirectory(path);

                        // break;
                    }
                    case gto:
                    {
                        if(C_Helper.InputPaths.size() == 1)
                        {
                            source = C_Helper.InputPaths[0];
                            source = GetRealPath(source);
                            //cout << source << endl;
                            //cin.get();

                            dir.PopulateDirectoryStructure(source);
                            storePathTraversal(source);
                            StatusMsg = "COMMAND MODE : In Directory : " + dir.DirPath;
                        }
                        else
                        {
                            StatusMsg = "COMMAND MODE : Invalid Command";
                        }

                        break;


                        // path = C_Helper.GetPath();
                        // if(path == "")
                        // {
                        //     PrintFilesInformation(1);
                        //     continue;
                        // }

                        // path = C_Helper.GetRealPath(path);
                        // dir.PopulateDirectoryStructure(path);

                        // StatusMsg = "COMMAND MODE : In Directory : " + dir.DirPath;
                        // // cout<<"After Print" <<endl;

                        // storePathTraversal(path);

                        // break;
                    }
                    case srch:
                    {
                        if(C_Helper.InputPaths.size() == 1)
                        {
                            source = C_Helper.InputPaths[0];

                            bool search = false;
                            //cout << "Path=" << dir.DirPath <<endl;
                            search = C_Helper.Search(dir.DirPath, source);
                            StatusMsg =  (search==true) ? "COMMAND MODE : True" : "COMMAND MODE : False";
                        }
                        else
                        {
                            StatusMsg = "COMMAND MODE : Invalid Command";
                        }

                        break;


                        // fName = C_Helper.GetName();
                        // // path = C_Helper.GetPath();
                        // if(fName == "")
                        // {
                        //     PrintFilesInformation(1);
                        //     continue;
                        // }

                        // path = C_Helper.GetRealPath(dir.DirPath);
                        // bool search = false;
                        // search = C_Helper.Search(path, fName);
                        // StatusMsg =  (search==true) ? "COMMAND MODE : True" : "COMMAND MODE : False";
                        // break;
                    }
                    case quit:
                    {
                        EchoSignalChange();
                        exit(1);
                        break;
                    }
                    default:
                    {
                        StatusMsg = "COMMAND MODE : Invalid command";
                        break;
                    }
                    }//Switch cmd_Type
                }
                
                PrintFilesInformation(1);
            }
        }

        } // Normal mode Switch case
    }

    return 0;
}

void storePathTraversal(string p_Path)
{
    if (PathTravsersalIndex != -1 && PathTravsersalIndex < PathTraversal.size() - 1)
    {
        while (PathTravsersalIndex != PathTraversal.size() - 1)
        {
            PathTraversal.pop_back();
        }
    }
    PathTraversal.emplace_back(GetRealPath(p_Path));
    PathTravsersalIndex++;
}

string GetRealPath(string path)
{
    char ch;
    int i = 1;
    char p[PATH_MAX];
    if(path[0] == '.')
    {
        path = dir.DirPath + "/" + path;
    }
    if(path[0] == '~')
    {
        path = "/home/" + USER + path.substr(1, path.length()-1);
    }
    realpath(path.c_str(), p);
    string str = tostring(p);
    return str;
}
string tostring(char *charr)
{
    string res;
    for (int i = 0; i < PATH_MAX; i++)
    {
        if (charr[i])
            res += charr[i];
        else
            break;
    }
    return res;
}

void changeTerminalSettings()
{
    struct termios newTerminalAttrs;
    if (!isatty(STDIN_FILENO))
    {
        cout << "Execute in terminal mode" << endl;
        exit(EXIT_FAILURE);
    }
    tcgetattr(STDIN_FILENO, &savedTerminalAttrs);
    atexit(resetTerminalSettings);

    tcgetattr(STDIN_FILENO, &newTerminalAttrs);
    newTerminalAttrs.c_lflag &= ~(ICANON | ECHO); /* Clear ICANON and ECHO. */
    // newTerminal.c_cc[VMIN] = 1;
    // newTerminal.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &newTerminalAttrs);
}

void resetTerminalSettings()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &savedTerminalAttrs);
}

void EchoSignalChange()
{
    struct termios newTerminalAttrs;
    tcgetattr(STDIN_FILENO, &newTerminalAttrs);
    newTerminalAttrs.c_lflag &= ~(ECHO); /* Clear ICANON and ECHO. */
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &newTerminalAttrs);

    // cout << "Echo signal enabled" << endl;
}

void modeToStr(mode_t mode, char *buf)
{
    const char chars[] = "rwxrwxrwx";
    for (size_t i = 0; i < 9; i++)
    {
        buf[i] = (mode & (1 << (8 - i))) ? chars[i] : '-';
    }
    buf[9] = '\0';
}
