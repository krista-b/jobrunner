#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <csse2310a3.h> 
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

/* Structure type to store the command line arguments. 
 */
typedef struct {
    int vPresent;
    int numFiles;
    char** jobFiles;
} CommandArgs;

/* Structure type to store information about a job.
 */
typedef struct {
    char* program;
    char* in;
    char* out;
    int timeout;
    char** args;
    int numArgs;
} JobInfo;

/* Structure type to store information about all the jobs in a file.
 */
typedef struct {
    char** jobs;
    int numParts;
    int numJobs;
} JobStruct;

/* Structure type to store information about the line numbers of the 
 * runnable lines in the job file. 
 */
typedef struct {
    int lines[50];
    int numLines;
} ValidLines;

/* Structure type to store information about the pipes in the jobfile that 
 * are valid i.e. have both in and out ends. 
 */
typedef struct {
    char* pipes[20];
    int numValidPipes;
} ValidPipes;

/* Structure type to store information about all pipes in the job file. 
 */
typedef struct {
    char* pipesIn[20];
    int numIn;
    char* pipesOut[20];
    int numOut;
} Pipes;

/* Checks if the line begins with a # and hence is a comment line. 
 *
 * Returns true if it is a comment and false otherwise. 
 */
bool is_comment(char* line) {
    return line[0] == '#';
}

/* Checks if there are no content in a line and hence is an empty line. 
 *
 * Returns false if there is a character in the line and true otherwise. 
 */
bool is_empty(char* line) {
    for (int i = 0; i < strlen(line); i++) {
        if (!isspace(line[i])) {
            return false;
        }
    }
    return true;
}

/* Checks a string to see if it is a positive number. 
 *
 * Returns false if the string contains an alpha character or negative number
 * and true otherwise. 
 */
bool is_pos_number(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (isdigit(str[i]) == 0) {
            return false;
        }
    }
    return true;
}

/* Checks validity of job line by checking there is enough information, 
 * all mandatory components are present and the timeout value is positive.
 *
 * Returns false if any of the above requirements are not met and 
 * true otherwise. 
 */
bool check_line(char** line, int num) {
    if (num < 3) {
        return false;
    }
    if (is_empty(line[0]) || is_empty(line[1]) || is_empty(line[2])) {
        return false;
    }
    if (num > 3) {
        if (!is_pos_number(line[3])) {
            return false;
        }
    }
    return true;
}

/* Parses the information provided in the line into a JobInfo struct.
 *
 * Returns Job.
 */
JobInfo parse_job(char** line, int num) {
    JobInfo job;
    job.program = line[0];
    if (!strcmp(line[1], "-")) {
        job.in = "stdin";
    } else {
        job.in = line[1];
    }
    if (!strcmp(line[2], "-")) {
        job.out = "stdout";
    } else {
        job.out = line[2];
    }
    if (num < 4) {
        job.timeout = 0;
        num++;
    } else {
        if (is_empty(line[3])) {
            job.timeout = 0;
        } else {
            job.timeout = atoi(line[3]);
        }
    }
    job.args = malloc(sizeof(char) * (num + 1));

    int numArgs = 0;
    if (num > 4) {
        for (int i = 0; i < num - 4; i++) {
            job.args[i] = line[i + 4];
            numArgs++;
        }
    }
    job.numArgs = numArgs;
    return job;
}

/* Reads the file line by line, splitting the line by commas, checking 
 * validity and adding this information to a JobStruct. 
 *
 * Returns JobStruct. 
 */
JobStruct read_files(CommandArgs args) {
    int numJobs = 0;
    int lineNum;
    JobInfo job;
    JobStruct jobs = {NULL};
    int k = 0;

    for (int i = 0; i < args.numFiles; i++) {
        lineNum = 0;
        FILE* file = fopen(args.jobFiles[i], "r");
        char* line;
        line = read_line(file);

        while (line) {
            lineNum++;
            if (is_comment(line) || is_empty(line)) {
                line = read_line(file);
                continue;
            }
            numJobs++;
            char** processed = split_by_commas(line);
            int j = 0;
            while (processed[j]) {
                j++;
            } 
            if (!check_line(processed, j)) {
                fprintf(stderr, "jobrunner: invalid job specification on"
                        " line %d of \"%s\"\n", lineNum, args.jobFiles[i]);
                exit(3);
            }
            job = parse_job(processed, j);
            jobs.jobs = realloc(jobs.jobs, sizeof(char*) * 
                    (job.numArgs + 4 + k + 1));
            int temp = k;
            for (int i = 0; i < job.numArgs + (j - job.numArgs); i++) {
                jobs.jobs[i + temp] = processed[i];
                k++;
            }
            jobs.jobs[k] = ".";
            k++;
            jobs.numParts = k;
            
            line = read_line(file);
        }
        jobs.numJobs = numJobs;
    }
    return jobs;
}

/* Checks for an error when opening the file. 
 *
 * Returns false if the file is not accessible and true otherwise. 
 */
bool file_accessible(char* filepath) {
    FILE* file = fopen(filepath, "r");
    if (!file) {
        return false;
    }
    fclose(file);
    return true;
}

/* Checks if all files passed to jobrunner in the command line are accessible. 
 * Prints the error message and exits the program with code 2 if any file 
 * is unaccessible.
 */
void check_files(CommandArgs args) {
    for (int i = 0; i < args.numFiles; i++) {
        if (!file_accessible(args.jobFiles[i])) {
            fprintf(stderr, "jobrunner: file \"%s\" can not be opened\n", 
                    args.jobFiles[i]);
            exit(2);
        }
    }
}

/* Parses the command line arguments into CommandArgs struct. 
 *
 * Returns CommandArgs. 
 */
CommandArgs parse_args(int argc, char** argv) {
    int v = 0;
    int c = 0;
    CommandArgs args;
    args.jobFiles = NULL;

    for (int i = 1; i < argc; i++) {
        if (v > 1) {
            fprintf(stderr, "Usage: jobrunner [-v] jobfile [jobfile ...]\n");
            exit(1);
        }
        if (!strcmp(argv[i], "-v")) {
            if (argv[i + 1] == NULL) {
                fprintf(stderr, "Usage: jobrunner [-v] "
                        "jobfile [jobfile ...]\n");
                exit(1);
            }
            v++;
            continue;
        }
        char* fileCopy;
        int len = strlen(argv[i]);
        fileCopy = malloc(sizeof(char) * (len + 1));
        strcpy(fileCopy, argv[i]);
        args.jobFiles = realloc(args.jobFiles, sizeof(char*) * (c + 1));
        args.jobFiles[c] = fileCopy;
        c++;
    }
    args.vPresent = v;
    args.numFiles = c;
    return args;
}

/* Prints to stderr the error message for bad pipe usage using the 
 * information passed in i.e. the bad pipe name.
 */
void bad_pipe(char* pipe) {
    fprintf(stderr, "Invalid pipe usage \"%s\"\n", &pipe[1]);
}

/* Reads the information provided in the jobs structure and adds all pipes to 
 * a pipes structure.
 *
 * Returns Pipes. 
 */
Pipes parse_pipes(JobStruct jobs) {
    Pipes pipes = {NULL};
    int j = 0;
    char* info[20];
    pipes.numIn = 0;
    pipes.numOut = 0;

    for (int i = 0; i < jobs.numParts; i++) {
        if (!strcmp(jobs.jobs[i], ".")) {
            JobInfo job = parse_job(info, j);
            if (job.in[0] == '@') {
                pipes.pipesIn[pipes.numIn] = job.in;
                pipes.numIn++;
            }
            if (job.out[0] == '@') {
                pipes.pipesOut[pipes.numOut] = job.out;
                pipes.numOut++;
            }
            j = 0;
        } else {
            info[j++] = jobs.jobs[i];
        }
    }
    return pipes;
}

/* Reads information provided in the jobs structure and adds the pipes 
 * to the provided valid pipes structure if it is valid and if it is not 
 * already present. 
 *
 * Returns updated ValidPipes. 
 */
ValidPipes pipe_validity(char* job, JobStruct jobs, ValidPipes validPipes, 
        int numPipes, char* pipes[]) {
    int c = 0;
    
    for (int j = 0; j < numPipes; j++) {
        if (!strcmp(job, pipes[j])) {
            c++;
        }
    }
    int exists = 0;
    if (c % 2) {
        for (int k = 0; k < validPipes.numValidPipes; k++) {
            if (!strcmp(job, validPipes.pipes[k])) {
                exists++;
            }
        }
        if (!exists) {
            validPipes.pipes[validPipes.numValidPipes] = job;
            validPipes.numValidPipes++;
        }
    } else {
        bad_pipe(job);
    }

    return validPipes;
}

/* Reads the information provided in the jobs structure and adds all valid 
 * pipes to a valid pipes structure.
 *
 * Returns ValidPipes. 
 */
ValidPipes valid_pipes(JobStruct jobs) {
    char* info[20];
    ValidPipes validPipes = {NULL};
    int j = 0;
    Pipes pipes = parse_pipes(jobs);
    if ((pipes.numIn + pipes.numOut) == 0) {
        return validPipes;
    }
    for (int i = 0; i < jobs.numParts; i++) {
        if (!strcmp(jobs.jobs[i], ".")) {
            JobInfo job = parse_job(info, j);
            if (job.in[0] == '@') {
                validPipes = pipe_validity(job.in, jobs, validPipes, 
                        pipes.numOut, pipes.pipesOut);
            }
            if (job.out[0] == '@') {
                validPipes = pipe_validity(job.out, jobs, validPipes, 
                        pipes.numIn, pipes.pipesIn);
            }
            j = 0;
        } else {
            info[j++] = jobs.jobs[i];
        }
    }
    return validPipes;
}

/* Checks if the provided pipe (inOut) is present in the provided valid pipes 
 * structure and if it is, the corresponding provided line number is added 
 * to the provided valid lines structure. 
 *
 * Returns updated ValidLines.
 */
ValidLines line_validity(ValidPipes pipes, ValidLines lines, char* inOut, 
        int lineNum) {
    for (int i = 0; i < pipes.numValidPipes; i++) {
        if (!strcmp(pipes.pipes[i], inOut)) {
            lines.lines[lines.numLines] = lineNum;
            lines.numLines++;
        }
    }
    return lines;
}

/* Reads the provided pipes and jobs information and checks the validity of 
 * each pipe on each line. If the line contains no pipes, 1 valid pipe and 1
 * file or 2 valid pipes then the line is added to the valid lines structure. 
 *
 * Returns ValidLines.
 */
ValidLines check_pipes(ValidPipes pipes, JobStruct jobs) {
    int j = 0, lineNum = 0;
    char* info[20];
    ValidLines lines;
    lines.numLines = 0;
    for (int i = 0; i < jobs.numParts; i++) {
        if (!strcmp(jobs.jobs[i], ".")) {
            lineNum++;
            JobInfo job = parse_job(info, j);
            if ((job.in[0] == '@') == 0 && (job.out[0] == '@') == 0) {
                lines.lines[lines.numLines] = lineNum;
                lines.numLines++;
            }
            if (job.in[0] == '@' && (job.out[0] == '@') == 0) {
                lines = line_validity(pipes, lines, job.in, lineNum);
            }
            if (job.out[0] == '@' && (job.in[0] == '@') == 0) {
                lines = line_validity(pipes, lines, job.out, lineNum);
            }
            int match = 0;
            if (job.in[0] == '@' && job.out[0] == '@') {
                for (int i = 0; i < pipes.numValidPipes; i++) {
                    if (!strcmp(pipes.pipes[i], job.in)) {
                        match = 1;
                    }
                    if (strcmp(pipes.pipes[i], job.out) == 0 && match > 0) {
                        lines.lines[lines.numLines] = lineNum;
                        lines.numLines++;
                    }
                }
                match = 0;
                for (int i = 0; i < pipes.numValidPipes; i++) {
                    if (!strcmp(pipes.pipes[i], job.out)) {
                        match = 1;
                    }
                    if (strcmp(pipes.pipes[i], job.in) == 0 && match > 0) {
                        lines.lines[lines.numLines] = lineNum;
                        lines.numLines++;
                    }
                }
            }
            j = 0;
        } else {
            info[j++] = jobs.jobs[i];
        }
    }
    return lines;
}

/* Forks the program, redirects stdin, stderr and stdout where neccessary 
 * and execs into the new program.  
 *
 * Returns the process id of the child created in fork. 
 */
int do_fork(JobInfo job, int child) {
    int p = fork();
    if (!p) { 
        // redirect stdin
        if (strcmp(job.in, "stdin")) {
            int inNo = open(job.in, O_RDONLY);
            dup2(inNo, STDIN_FILENO);
            close(inNo);
        }
        
        // redirect stdout
        if (strcmp(job.out, "stdout")) {
            int outNo = open(job.out, O_WRONLY | O_CREAT | O_TRUNC);
            dup2(outNo, STDOUT_FILENO);
            close(outNo);
        }

        // redirect stderr
        int errNo = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC);
        dup2(errNo, STDERR_FILENO);
        close(errNo);

        char* args[job.numArgs + 2];
        args[0] = job.program;
        for (int i = 0; i < job.numArgs; i++) {
            args[i + 1] = job.args[i];
        }
        args[job.numArgs + 1] = NULL;

        execvp(args[0], args);
        fprintf(stderr, "Job %d exited with status %d\n", child, 255);
    }
    return p;
}

/* Waits on all children processes and prints the appropriate exit message 
 * when the child either exits normally or exits with a signal. 
 */
void do_wait(int lastChild, int numChildren, int jobs) {
    int status;
    
    for (int i = lastChild; i < lastChild + numChildren; i++) {
        pid_t pid = waitpid(-1, &status, 0);
        if (pid == -1) {
            exit(255);
        }

        int child = pid - (lastChild - jobs);

        if (WIFEXITED(status)) {
            const int es = WEXITSTATUS(status);
            fprintf(stderr, "Job %d exited with status %d\n", child, es);

        } 
        if (WIFSIGNALED(status)) {
            fprintf(stderr, "Job %d exited with status %d\n", child, 1);
        }
    }
}

/* Checks that provided jobs' input file, if not stdin, is openable for reading
 * and outputfile, if not stdout, is openable for writing. 
 *
 * Returns 0 if either file is not openable and 1 otherwise. 
 */
int check_job_files(JobInfo job) {
    int n = 1;
    if (strcmp(job.in, "stdin")) {
        if (!file_accessible(job.in)) {
            fprintf(stderr, "Unable to open \"%s\" for reading\n", job.in);
            n = 0;
        }
    }
    
    if (strcmp(job.out, "stdout")) {
        int file = open(job.out, O_WRONLY | O_CREAT | O_TRUNC, 
                S_IRUSR | S_IWUSR);
        if (file < 0) {
            fprintf(stderr, "Unable to open \"%s\" for writing\n", job.out);
            n = 0;
        }
    }
    
    return n;
}

/* Reads information provided for each job within the jobs structure and calls
 * do_fork if the line is valid i.e. present in the provided valid lines 
 * structure. After any valid jobs have been run, do_wait is called on each. 
 *
 * Returns the number of jobs. 
 */
int do_jobs(JobStruct jobs, ValidLines validLines, CommandArgs args) {
    char* info[20];
    int lastChild = 0;
    int j = 0, jobsDone = 0, doJob = 0, lines = 0, children = 0;

    for (int i = 0; i < jobs.numParts; i++) {
        if (!strcmp(jobs.jobs[i], ".")) {
            lines++;
            jobsDone++;
            for (int k = 0; k < validLines.numLines; k++) {
                if (lines == validLines.lines[k]) {
                    doJob = 1;
                }
            } 

            JobInfo job = parse_job(info, j);
            if (doJob) {
                if (check_job_files(job)) {
                    if (args.vPresent) {
                        fprintf(stderr, "%d:%s:%s:%s:%d", lines, job.program, 
                                job.in, job.out, job.timeout);
                        if (job.numArgs) {
                            for (int i = 0; i < job.numArgs; i++) {
                                fprintf(stderr, ":%s", job.args[i]);
                            }
                        }
                        fprintf(stderr, "\n");
                    }
                    lastChild = do_fork(job, children + 1);
                    children++;
                } else {
                    jobsDone--;
                }
            }
            j = 0;
            doJob = 0;
        } else {
            info[j++] = jobs.jobs[i];
        }
    }
    if (jobsDone) {
        do_wait(lastChild, children, jobsDone);
    }
    return jobsDone;
}

/* Main function. Prints usage error if there are not enough arguments 
 * provided, otherwise the arguments are assigned, checked for validity, and 
 * parsed into the appropriate information i.e. jobs, pipes and lines. 
 * Calls do_job for each valid job line and prints appropriate error message
 * if no jobs are valid. 
 *
 * Returns 4 if no jobs are runnable and 0 otherwise. 
 */
int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: jobrunner [-v] jobfile [jobfile ...]\n");
        return 1;
    }

    CommandArgs args;
    args = parse_args(argc, argv);

    check_files(args);
    JobStruct jobs = {NULL};
    jobs = read_files(args);
    ValidPipes validPipes = {NULL};
    ValidLines validLines;
    validPipes = valid_pipes(jobs);

    int numJobs = 0;
    validLines = check_pipes(validPipes, jobs);
    if (validLines.numLines) {
        if (jobs.numJobs) {
            numJobs = do_jobs(jobs, validLines, args);
        }
    }
    if (numJobs == 0) {
        fprintf(stderr, "jobrunner: no runnable jobs\n");
        return 4;
    }

    return 0;
}
