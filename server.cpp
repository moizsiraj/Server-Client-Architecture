#include <iostream>
#include <unistd.h>
#include <regex>
#include <cstdio>
#include <cstring>
#include <wait.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

int setOperation(char *operationText);

void signal_handler(int signo);

bool checkFormat(char *input);

std::string getTime();

std::string elapsedTime(std::string startTime, std::string endTime);

int removeColon(std::string s);

void createSock();

double performOperation(int saveCurrentNumber, double currentTotal,
                        int operation);//method performs the operation on the given integer

int killProcess(char *PID);

int runProcess(char *processName, char *filePath);

bool getFirstNumber;
std::string list[10][6];
int noOfCurrentProcess = 0;
int sock;
char inputText[500];
char outputText[500];
int msgsock;
bool divZero = false;

int main() {

    bool continueInput = true;
    char saveOperator[10];
    int operation = -1;
    char *token;


    createSock();
    listen(sock, 5);
    msgsock = accept(sock, 0, 0);
    if (msgsock != -1) {
        write(msgsock, "Commands: kill <pid>, list, run <process> <path(optional)>, "
                       "add/div/sub/mul <list of numbers separated by spaces>\nInput exit to terminate:\n"
                       "Please input your command:\n", 166);
    }
    if (signal(SIGCHLD, signal_handler) == SIG_ERR) {
        write(STDOUT_FILENO, "sig error", 9);
    }

    while (continueInput) {
        int checkRead = read(msgsock, inputText, 500);
        inputText[checkRead - 1] = '\0';
        if (checkRead == 1) {//empty input
            write(msgsock, "Input next command\n", 19);
            continue;
        }
        token = strtok(inputText, " ");
        sscanf(token, "%s", saveOperator);
        operation = setOperation(saveOperator);
        if (operation == 0) {
            continueInput = false;
            int *status = nullptr;
            write(msgsock, "exit\0", 5);
            close(sock);
            close(msgsock);
            wait(status);
            kill(getpid(), SIGTERM);
        } else if (operation == -1) {
            write(msgsock, "Invalid command.\nInput next command\n", 36);
        } else if (operation >= 1 && operation <= 4) {
            int saveCurrentNumber;
            bool invalidInput = false;
            double total = 0;
            getFirstNumber = false;
            char checkInteger[10];
            token = strtok(nullptr, " ");
            while (token != nullptr) {
                sscanf(token, "%d", &saveCurrentNumber);
                sscanf(token, "%s", checkInteger);
                if (checkFormat(checkInteger)) {
                    total = performOperation(saveCurrentNumber, total, operation);
                } else {
                    invalidInput = true;
                }
                token = strtok(nullptr, " ");
            }
            int noOfCharPrint = sprintf(outputText, "%.5f \n", total);
            int count;
            if (invalidInput) {
                int noOfChars = sprintf(&outputText[noOfCharPrint - 1], "%s",
                                        "\nOnly Integer values considered for calculations. Others were ignored.\nInput next command\n");
                count = noOfCharPrint + noOfChars;
            } else if (divZero) {
                int noOfChars = sprintf(&outputText[noOfCharPrint - 1], "%s",
                                        "\nDivision by Zero. Invalid Operation.\nInput next command\n");
                count = noOfCharPrint + noOfChars;
            } else {
                int noOfChars = sprintf(&outputText[noOfCharPrint - 1], "%s", "\nInput next command\n");
                count = noOfCharPrint + noOfChars;
            }

            write(msgsock, outputText, count);
        } else if (operation == 5) {//Kill
            char *processPID;
            processPID = strtok(nullptr, " ");
            if (processPID == nullptr) {
                write(msgsock, "Input next command\n", 19);
            } else {
                int killCheck = killProcess(processPID);
                if (killCheck == 1) {
                    write(msgsock, "Invalid pid\nInput next command\n", 31);
                } else if (killCheck == 0) {
                    write(msgsock, "Process killed\nInput next command\n", 34);
                } else {
                    write(msgsock, "Process already killed.\nInput next command\n", 43);
                }
            }
        } else if (operation == 6) {//Run
            char *processName;
            processName = strtok(nullptr, " ");
            char *filePath;
            filePath = strtok(nullptr, " ");
            if (processName != nullptr) {
                runProcess(processName, filePath);
            } else {
                write(msgsock, "Input next command\n", 19);
            }
        } else if (operation == 7) {//List
            char output[500];
            std::string print;
            print.append("Process PID\tProcess Name\tStatus\t\tStart Time\t\tEnd Time\t\tElapsed Time\n\n");
            for (int i = 0; i < noOfCurrentProcess; ++i) {
                for (int j = 0; j < 6; ++j) {
                    print.append(list[i][j]).append("\t\t");
                }
                print.append("\n");
            }
            int read = sprintf(output, "%s", print.c_str());
            sprintf(&output[read - 1], "%s", "\nInput next command\n");
            int count = read + 20;

            write(msgsock, output, count);
        } else if (operation == 8) {
            token = strtok(nullptr, " ");
            char messageBuffer[500];
            std::string print;
            while (token != nullptr) {
                print.append(token).append(" ");
                token = strtok(nullptr, " ");
            }
            print.append("\n");
            int read = sprintf(messageBuffer, "%s", print.c_str());
            write(STDOUT_FILENO, messageBuffer, read);
            write(msgsock, "\nInput next command\n", 20);
        }
    }
    return 0;
}

void createSock() {
    char output[500];
    struct sockaddr_in server;//struct to store socket info
    int length;
    sock = socket(AF_INET, SOCK_STREAM, 0);//socket created
    if (sock < 0) {
        perror("opening stream socket");
        exit(1);
    } else {//setting values in the structure
        server.sin_family = AF_INET;//for communication over the internet
        server.sin_addr.s_addr = INADDR_ANY;//can connect to any address
        server.sin_port = 0;//passing 0 so system can assign any port number
    }
    if (bind(sock, (struct sockaddr *) &server, sizeof(server))) {//binding socket with the port
        perror("binding stream socket");
        exit(1);
    }
    length = sizeof(server);
    if (getsockname(sock, (struct sockaddr *) &server, (socklen_t *) &length)) {// getting the assigned port
        perror("getting socket name");
        exit(1);
    }
    char *port;
    int portNo = ntohs(server.sin_port);
    int noOfChars = sprintf(output, "%s", "Socket has port #\n");
    int portChars = sprintf(&output[noOfChars - 1], "%d\n", portNo);
    int count = noOfChars + portChars;
    write(STDOUT_FILENO, output, count);
    fflush(stdout);
}

int runProcess(char *processName, char *filePath) {
    int pipefds3[2];
    int pipeCheck = pipe2(pipefds3, O_CLOEXEC);
    int pidChild2 = fork();
    if (pidChild2 == 0) {
        close(pipefds3[0]);
        int execCheck = execlp(processName, processName, filePath, NULL);
        if (execCheck == -1) {
            if (errno == EACCES) {
                write(pipefds3[1], "User does not have access right for the file\n", 46);
            } else if (errno == EFAULT) {
                write(pipefds3[1], "File outside user accessible memory\n", 36);
            } else if (errno == ENOENT) {
                write(pipefds3[1], "File does not exist\n", 20);
            } else {
                write(pipefds3[1], "Error\n", 6);
            }
            kill(getpid(), SIGTERM);
        }
    }
    if (pidChild2 > 0) {
        close(pipefds3[1]);
        char errorCheck[50];
        list[noOfCurrentProcess][0] = std::to_string(pidChild2);
        list[noOfCurrentProcess][1] = processName;
        list[noOfCurrentProcess][2] = "Running";
        list[noOfCurrentProcess][3] = getTime();
        list[noOfCurrentProcess][4] = "\t-";
        list[noOfCurrentProcess][5] = "\t-";
        noOfCurrentProcess++;
        bool error = false;
        int readCheck = read(pipefds3[0], errorCheck, 50);
        if (readCheck > 0) {
            error = true;
        }
        if (error) {
            int noOfChars = sprintf(&errorCheck[readCheck - 1], "%s", "\nInput next command\n");
            noOfCurrentProcess--;
            int count = readCheck + noOfChars;
            write(msgsock, errorCheck, count);
        } else {

            write(msgsock, "Input next command\n", 19);
        }
    }
    return 0;
}

std::string getTime() {
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
    std::string str(buffer);
    return str;
}

int killProcess(char *PID) {
    int pid = atoi(PID);
    int index = -1;
    for (int i = 0; i < noOfCurrentProcess; ++i) {
        if (strcmp(PID, list[i][0].c_str()) == 0) {
            index = i;
            break;
        }
    }
    if (index != -1 && strcmp(list[index][2].c_str(), "Running") == 0) {
        kill(pid, SIGTERM);
        list[index][2] = "Killed";
        list[index][4] = getTime();
        list[index][5] = elapsedTime(list[index][3], list[index][4]);
        return 0;
    } else if (index != -1 && strcmp(list[index][2].c_str(), "Killed") == 0) {
        return -1;
    } else {
        return 1;
    }
}

double performOperation(int saveCurrentNumber, double currentTotal, int operation) {
    if (!getFirstNumber) {
        currentTotal = saveCurrentNumber;
        getFirstNumber = true;
    } else {
        switch (operation) {
            case 1:
                currentTotal = currentTotal + saveCurrentNumber;
                break;
            case 2:
                currentTotal = currentTotal - saveCurrentNumber;
                break;
            case 3:
                currentTotal = currentTotal * saveCurrentNumber;
                break;
            case 4:
                if (saveCurrentNumber == 0) {
                    divZero = true;
                } else {
                    currentTotal = currentTotal / saveCurrentNumber;
                }
                break;
        }
    }
    return currentTotal;
}

int setOperation(char *operationText) {
    int operation;
    if (strcmp(operationText, "add") == 0) {
        operation = 1;
    } else if (strcmp(operationText, "sub") == 0) {
        operation = 2;
    } else if (strcmp(operationText, "mul") == 0) {
        operation = 3;
    } else if (strcmp(operationText, "div") == 0) {
        operation = 4;
    } else if (strcmp(operationText, "kill") == 0) {
        operation = 5;
    } else if (strcmp(operationText, "run") == 0) {
        operation = 6;
    } else if (strcmp(operationText, "list") == 0) {
        operation = 7;
    } else if (strcmp(operationText, "exit") == 0) {
        operation = 0;
    } else if (strcmp(operationText, "print") == 0) {
        operation = 8;
    } else {
        operation = -1;
    }
    return operation;
}

bool checkFormat(char *input) {
    bool isNumber;
    std::regex b("^[-+]?\\d+$");//accepts negative and positive integers only
    if (regex_match(input, b)) {
        isNumber = true;
    } else {
        isNumber = false;
    }
    return isNumber;
}

int removeColon(std::string s) {
    char buffer[8];
    int count = sprintf(buffer, "%s", s.c_str());
    std::replace(s.begin(), s.end(), ':', '0');
    count = sprintf(buffer, "%s", s.c_str());
    return std::stoi(s);
}

std::string elapsedTime(std::string startTime, std::string endTime) {
    int time1 = removeColon(startTime);
    int time2 = removeColon(endTime);

    int hourDiff = time2 / 1000000 - time1 / 1000000 - 1;
    time1 = time1 % 1000000;
    time2 = time2 % 1000000;

    int minDiff = time2 / 1000 + (60 - time1 / 1000);
    if (minDiff >= 60) {
        hourDiff++;
        minDiff = minDiff - 60;
    }

    time1 = time1 % 100;
    time2 = time2 % 100;

    int secDiff = (time2) + (60 - time1);
    if (secDiff >= 60) {
        secDiff = secDiff - 60;
    }
    std::string res = std::to_string(hourDiff) + ':' + std::to_string(minDiff) + ':' + std::to_string(secDiff);
    return res;
}

void signal_handler(int signo) {
    if (signo == SIGCHLD) {
        int status;
        int pid = waitpid(0, &status, WNOHANG);
        if (pid != 0) {
            int index = -1;
            for (int i = 0; i < noOfCurrentProcess; ++i) {
                if (pid == stoi(list[i][0])) {
                    index = i;
                    break;
                }
            }
            if (index != -1 && strcmp(list[index][2].c_str(), "Running") == 0) {
                list[index][2] = "Killed";
                list[index][4] = getTime();
                list[index][5] = elapsedTime(list[index][3], list[index][4]);
            }
        }
    }
}