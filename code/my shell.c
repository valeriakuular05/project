#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

#define size 4
#define ncmd 150
#define maxlen 500
#define commandLen 150
#define dlen 100 // для seqCommand папка

typedef struct seq{
    char *s;
    int recur;
} sequen;

typedef struct seq_ar{
    sequen* arr;
    int len;
} sequen_arr;

typedef struct condition{
    char* str;
    int recur;
    int prevSuccess;
} if_elem;

typedef struct if_ar{
    if_elem *arr;
    int count;
} if_array;


/*---------------------------------Function-----------------------*/

char *get_str();
sequen_arr pars(char*);
pid_t seqCommand(sequen_arr);
if_array condit_check(char*);
if_elem st_if_elem(char*, char*);
sequen str_to_seq(char*);
sequen_arr conv_pars(char*);
pid_t simple(char* , int, int);
int conv(sequen_arr);
pid_t if_com(if_array);

/*---------------------------------------------------------------*/

if_elem st_if_elem(char* str, char* operat){
    int len = strlen(str);
    while (1){
        if (str[len - 1] == ' '){len--; str[len] = '\0';}
        else break;
    }
    int f_staples = 0;
    if_elem res;
    res.str = calloc(commandLen, 1);
    for (int i = 1; i < (len - 1); i++){
        if (str[i] == '(') f_staples++;
        if (str[i] == ')') f_staples--;
        if (f_staples < 0){
            break;
        }
    }
    if ((str[0] == '(') && (f_staples == 0)){ // если (cmd1 && cmd2) - переделаем в cmd1 && cmd2
        for (int i = 1; i < (len - 1); i++){
            res.str[i - 1] = str[i];
        }
        res.recur = 1;
    }
    else{
        strcpy(res.str, str);
        res.recur = 0;
    }
    if (strcmp(operat, "&&") == 0) res.prevSuccess = 1; // проверяем оператор стоящий перед командой
    else res.prevSuccess = 0;
    return res;
}


if_array condit_check(char* s){ //команда cmd1 ...; разбиваем одну команду на подкоманды и исследуем условное выполнение
    if_elem* arr = NULL;
    int count = 0, f_staples = 0;
    char* word = calloc(commandLen, 1);
    char* cmd = calloc(commandLen, 1); 
    int cur = 0, and_check = 0, or_check = 0;
    char buf[3] = "&&";
    int n = strlen(s);
    for (int i = 0; i < n; i++){
        if (s[i] ==' '){
            if (s[i - 1] != ' '){
                and_check = strcmp(word, "&&");
                or_check = strcmp(word, "||");
                if (((and_check == 0) || (or_check == 0)) && (f_staples == 0)){
                    if_elem elem = st_if_elem(cmd, buf); //отправляем например - buf: || cmd: cmd2
                    strcpy(buf, word);
                    count++; // увеличиваем счетчик команд
                    arr = realloc(arr, count * sizeof(if_elem));
                    arr[count - 1] = elem;
                    free(word);
                    free(cmd);
                    word = calloc(commandLen, 1);
                    cmd = calloc(commandLen, 1);
                    cur = 0;
                
                    continue;
                }
                if (cmd[0]) strcat(cmd, " ");
                strcat(cmd, word);
                free(word);
                word = calloc(commandLen, 1); cur = 0; 
                continue;
            }
            else {
            continue;}
        }
        if (s[i] == ')'){
            f_staples--;
            word[cur] = s[i];
            if (cmd[0]) strcat(cmd, " ");
            strcat(cmd, word); free(word);
            word = calloc(commandLen, 1); cur = 0; 
            continue;
        }
        if (s[i] == '('){
            and_check = strcmp(word, "&&");
            or_check = strcmp(word, "||");
            if (((and_check == 0) || (or_check == 0)) && (f_staples = 0)){
                if_elem elem = st_if_elem(cmd, buf);
                strcpy(buf, word);
                count++;
                arr = realloc(arr, count * sizeof(if_elem));
                arr[count - 1] = elem; 
                free(word); free(cmd);
                word = calloc(commandLen, 1); cmd = calloc(commandLen, 1);
                word[0] = s[i];
                cur = 1; f_staples++;
                continue;
            }
            f_staples++;
            word[cur] = s[i];
            cur++;
            continue;
        }
        if (s[i] != '\n'){
            word[cur] = s[i];
            cur++;
        }
    }
    if (cmd[0]) strcat(cmd, " ");
    strcat(cmd, word);
    //printf("FFF%s\n", buf);
    //printf("FFF%s\n", cmd);
    if (cmd[0]){
        count++;
        arr = realloc(arr, sizeof(if_elem) * (count));
        if_elem elem;
        elem = st_if_elem(cmd, buf);
        //printf("suc %d\n", elem.prevSuccess);
        arr[count - 1] = elem;
    }
    if_array array;
    array.arr = arr;
    array.count = count;
    free(cmd);
    free(word);
    return array;
}


sequen_arr conv_pars(char* s){
    sequen* init = NULL; //ячейки
    int count = 0, f_staples = 0;
    char* new = calloc(commandLen, 1);
    char* cmd = calloc(commandLen, 1);
    int cur = 0, check = 0, n;
    for (int i = 0; i < strlen(s); i++){
        if (s[i] == ' '){
            if (i == 0) continue;
            if (s[i - 1] != ' '){
                check = strcmp(new, "|");
                if ((check == 0) && (f_staples == 0)){
                    n = strlen(cmd);
                    while(1){
                        if (cmd[n - 1] == ' '){ n--; cmd[n] = '\0';}
                        else break;
                    }
                    sequen elem = str_to_seq(cmd);
                    count++; 
                    init = realloc(init, sizeof(sequen) * count);
                    init[count - 1] = elem;
                    free(new); free(cmd);
                    new = calloc(commandLen, 1); cmd = calloc(commandLen, 1);
                    cur = 0;
                    continue;
                }
                if (cmd[0]) strcat(cmd, " ");
                strcat(cmd, new);
                free(new);
                new = calloc(commandLen, 1);
                cur = 0;
                continue;
            }
            else continue;
        }
        if (s[i] == ')'){
            f_staples--;
            new[cur] = ')';
            if (cmd[0]) strcat(cmd, " ");
            strcat(cmd, new);
            free(new); new = calloc(commandLen, 1);
            cur = 0;
            continue;
        }
        if (s[i] == '('){
            check = strcmp(new, "|");
            if ((check == 1) && (f_staples == 0)){
                n = strlen(cmd);
                while(1){
                    if (cmd[n - 1] == ' '){ n--; cmd[n] = '\0';}
                    else break;
                }
                sequen elem = str_to_seq(cmd);
                count++;
                init = realloc(init, sizeof(sequen) * count);
                init[count - 1] = elem;
                free(cmd); free(new);
                new = calloc(commandLen, 1);
                cmd = calloc(commandLen, 1);
                new[0] = s[i];
                cur = 1; f_staples++;
                continue;
            }
            f_staples++;
            new[cur] = s[i];
            cur++;
            continue;
        }
        if (s[i] != '\n'){
        new[cur] = s[i];
        cur++;
        }
    }
    if (cmd[0]) strcat(cmd, " ");
    strcat(cmd, new);
    if (cmd[0]){
        cur++;
        n = strlen(cmd);
        while(1){
            if (cmd[n - 1] == ' '){n--; cmd[n] = '\0';}
            else break;
        }
        count++;
        init = realloc(init, sizeof(sequen) * count);
        init[count - 1] = str_to_seq(cmd);
    }
    //printf("LEN CONV %s\n", init[0].s);
    sequen_arr array;
    array.arr = init;
    array.len = count;
    free(new);
    free(cmd);
    return array;
}

void chdirect(char *dir)
{
    chdir(dir);
}

pid_t simple(char *args,int input,int output)
{
    pid_t pid;
    if (args[0]=='c' && args[1]=='d')
    {
        if (args[2]=='\n' || args[2]=='\0') 
        {
            chdir("/");
        }
        char *dir = args + 3;
        chdirect(dir);
        return 1;
    }
    if (!(pid = fork()))
    {
        //signal(SIGINT, SIG_DFL);
        char** argv;
        argv = calloc(commandLen, sizeof(char*));
        for (int i = 0;i < commandLen; i++)
        {
            argv[i] = calloc(commandLen, sizeof(char));
        }
        int n = strlen(args);
        int pointer = 0;
        int ppointer = 0;
        for (int i = 0; i < n; i++)
        {
            if (args[i] == ' ')
            {
                if (args[i - 1] == ' ') continue;
                pointer++;
                ppointer = 0;
                continue;
            }
            if (args[i] == '\n') break;
            argv[pointer][ppointer] = args[i];
            ppointer++;
        }
        pointer++;
        for (int i = pointer; i < commandLen; i++) free(argv[i]);
        argv[pointer] = NULL;
        argv = realloc(argv, sizeof(char*)*(pointer+1));
        argv[pointer] = NULL;
        if (input)
        {
            dup2(input, 0);
            close(input);
        }
        if (output != 1)
        {
            dup2(output, 1);
            close(output);
        }
        //printf("ВЫВОД %s\n", args);
        execvp(argv[0], argv);
        //printf("ОШИБКА\n");
        for (int i = 0; i < pointer; i++) free(argv[i]);
        free(argv);
        exit(1);
    }
    int status;
    //int a;
    //waitpid(pid, &status, 0);
    // if (WIFEXITED(status)) {
    //     if (WEXITSTATUS(status) != 0) printf("Ошибка\n");
    //     return pid;
    // }
    return pid;
}


int conv(sequen_arr mas){
    //printf("CONV %s\n", mas.arr[0].s);
    int n = mas.len;
    int in = 0;
    int out = 1;
    int status = 1;
    int len, flag;
    if (status){
        char* word1 = calloc(commandLen, 1);
        char* word2 = calloc(commandLen, 1);
        int i = 0;
        while((mas.arr[0].s[i] != ' ') && (mas.arr[0].s[i] != '\0') && (mas.arr[0].s[i] != '\n')){
            word1[i] = mas.arr[0].s[i];
            i++;
        }
        while (mas.arr[0].s[i] == ' '){
            i++;
        }
        int tmp = i;
        int j = 0;
        while ((mas.arr[0].s[i] != ' ') && (mas.arr[0].s[i] != '\0') && (mas.arr[0].s[i] != '\n')){
            word2[j] = mas.arr[0].s[i];
            j++; i++;
        }
        if (word1[0] == '<'){
            for (int k = 0; k < strlen(mas.arr[0].s); k++) word1[k] = word1[k + 1];
            in = open(word1, O_RDONLY);
            for (int k = 0;k < strlen(mas.arr[0].s); k++) mas.arr[0].s[k] = mas.arr[0].s[k + tmp];
            if (word2[0] == '>'){
                flag = 0;
                for (int i = 0; i < strlen(word2); i++) word2[i] = word2[i + 1];
                if (word2[0] == '>'){
                    len = strlen(word2);
                    for (int i = 0; i < len; i++) word2[i] = word2[i + 1];
                    flag = 1;
                }
            if (flag == 0) out = open(word2,O_CREAT | O_WRONLY | O_TRUNC, 0666);
            else out = open(word2, O_CREAT | O_WRONLY | O_APPEND, 0666);
            for (int i = 0; i < strlen(mas.arr[0].s); i++) mas.arr[0].s[i] = mas.arr[0].s[i + strlen(word2) + 2];
            }
        
        }
        else if (word1[0] == '>')
        {
            flag = 0;
            int n = strlen(word1);
            for (int k = 0; k < strlen(word1); k++) word1[k] = word1[k + 1];
            if (word1[0] == '>'){
                n = strlen(word1);
                for (int k = 0; k < n; k++) word1[k] = word1[k + 1];
                flag = 1;
            }
            if (flag == 0) out = open(word1,O_CREAT | O_WRONLY | O_TRUNC, 0666);
            out = open(word1, O_CREAT | O_WRONLY | O_APPEND, 0666);
            //printf("%d %s\n", out, word1);
            for (int i = 0; i < strlen(mas.arr[0].s); i++) mas.arr[0].s[i] = mas.arr[0].s[i + tmp];
            if (word2[0] == '<'){
                for (int i = 0; i < strlen(word2); i++) word2[i] = word2[i + 1];
                //printf("%s\n", word2);
                in = open(word2, O_RDONLY);
                for (int i = 0; i < strlen(mas.arr[0].s); i++) mas.arr[0].s[i] = mas.arr[0].s[i + strlen(word2) + 2];
                //printf("%s\n", mas.arr[0].s);
            }
        } 
    }
    if (mas.len == 1){
        //signal(SIGINT, SIG_IGN);
        pid_t res = simple(mas.arr[0].s, in, out);
        int status;
        waitpid(res, &status, 0);
        signal(SIGINT, SIG_DFL);
        printf("AAAAAAA%s", mas.arr[0].s);
        printf("%d\n", status);
        return (!WIFEXITED(status) || WEXITSTATUS(status)); //не равно нулю, если дочерний процесс успешно завершился, возвращает восемь младших битов значения, которое вернул завершившийся дочерний процес
        }
    int fd[n - 1][2];
    pipe(fd[0]); int pids[n];
    pids[0] = simple(mas.arr[0].s,in,fd[0][1]);
    close(fd[0][1]);
    //printf("%s\n", mas.arr[0].s);
    //printf("%s\n", mas.arr[1].s);
    for (int i = 1; (i < (n - 1)); i++)
    {
        pipe(fd[i]);
        pids[i] = simple(mas.arr[i].s, fd[i-1][0], fd[i][1]);
        close(fd[i][1]);
        close(fd[i-1][0]);
    }
    pids[n-1] = simple(mas.arr[n-1].s, fd[n-2][0], out);
    close(fd[n-2][0]);
    for (int i = 0;i < (n-1); i++)
    {
        if (mas.arr[0].s[0]=='c' && mas.arr[0].s[1]=='d') continue;
        waitpid(pids[i], 0, 0);

    }
    int status_2;
    if (mas.arr[n-1].s[0]=='c' && mas.arr[n-1].s[1]=='d') goto func;
    waitpid(pids[n-1], &status_2, 0);
    printf("EXIT %d", WEXITSTATUS(status_2));
    if (WIFEXITED(status_2)){
        if (WEXITSTATUS(status_2) != 0){
            printf("FFFFFFFОшибка\n");
            
        }
    }
    return (!WIFEXITED(status_2) || WEXITSTATUS(status_2));
func:
    return 0;
}

pid_t if_com(if_array mas){
    int len = mas.count;
    int finish_status = 0;
    int ind = 0;
    if (len == 1){
        if (mas.arr[0].recur == 1) return seqCommand(pars(mas.arr[0].str));
        return conv(conv_pars(mas.arr[0].str));
    }
    for (int i = 0; (i < len) && ((mas.arr[i].prevSuccess != finish_status)); i++){
        if (mas.arr[i].recur == 1){
            int fd[2]; pipe(fd);
            char path[dlen];
            getcwd(path, dlen);
            pid_t pid = fork();
            if (pid == 0){
                close(fd[0]);
                //printf("FFFF%s\n", mas.arr[i].str);
                finish_status = seqCommand(pars(mas.arr[i].str));
                write(fd[1], &finish_status, sizeof(int));
                exit(0);
            }
            chdir(path);
            close(fd[0]);
            read(fd[1], &finish_status, sizeof(int));
            close(fd[1]);
            wait(0);
        }
        else finish_status = conv(conv_pars(mas.arr[i].str));
        //printf("Fin %d\n", finish_status);
        ind = i + 1;
    }
    if (ind != len) return 0;
    return 1;
}


pid_t seqCommand(sequen_arr mas){ // обработка структуры из массива условных ячеек 
                                // с командой изначальных и количества команд - обрабатываем каждую ячейку
    int l = mas.len;
    int status;
    for (int i = 0; i < mas.len; i++){
        if (mas.arr[i].recur == 1){
            //printf("SEQ %s\n", mas.arr[i].s);
             // делаем рекурсию если встретились скобки внутри команды
            char path[dlen]; // запоминаем директорию где мы были до выполнения команды в скобках
            getcwd(path, dlen);
            int fd[2]; pipe(fd);
            pid_t pid = fork();
            if (pid == 0){
                close(fd[0]);
                status = seqCommand(pars(mas.arr[i].s));
                write(fd[1], &status, sizeof(int));
                exit(0);
            }
            chdirect(path);
            close(fd[1]);
            read(fd[0], &status, sizeof(int));
            close(fd[0]);
            wait(0);
        }
        else{
            if_array array = condit_check(mas.arr[i].s);
            status = if_com(array);
        }
    }
    return status;
}

sequen str_to_seq(char* str){
    int len = strlen(str), countBreak = 0;
    sequen res;
    res.s = calloc(commandLen, 1);
    for (int i = 1; i < len - 1; i++){
        if (str[i] == '(') countBreak++;
        if (str[i] == ')') countBreak--;
        if (countBreak < 0){
            countBreak = -1;
            break;
        }
    }
    if ((str[0] == '(') && (countBreak == 0)){
        for (int i = 1; i < len - 1; i++){
            res.s[i - 1] = str[i];
        }
        res.recur = 1;
    }
    else{
        strcpy(res.s, str);
        res.recur = 0;
    }
    return res;
}

sequen_arr pars(char* str){ //парсим командную строчку на подкоманды и анализируем
                            // нужна ли рекурсия(в случае скубок - нужна для раскрытия)
                            // составляем структуру из массива структур(команда, рекур) и длины команд в строке
    int f_staples = 0; // >0  - открытых
    int pointer = 0;  // Начало текущей подстроки
    int sub_count = 0;  // Счетчик подстрок
    sequen* masSeq = NULL;
    char* cmd = calloc(commandLen, 1);
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] == ' '){
            if (i == 0) continue;
            if ((str[i - 1] == ' ') || (str[i - 1] == ')')) continue;
            if (pointer == 0) continue;
        }
        if (str[i] == '(') f_staples++;
        if (str[i] == ')') f_staples--;
        if ((str[i] == ';') && f_staples == 0) {
            if (cmd[pointer - 1] == ' ') cmd[pointer] = '\0';
            cmd[pointer] = '\0';
            sequen element = str_to_seq(cmd);
            sub_count++;
            masSeq = realloc(masSeq, sizeof(sequen) * sub_count);
            masSeq[sub_count - 1] = element;
            free(cmd);
            cmd = calloc(commandLen, 1);
            pointer = 0;
            continue;
        }
        if (str[i] != '\n'){
            cmd[pointer] = str[i];
            pointer++;
        }
    }
    if (cmd[0]){ // последняя команда в строке
        sub_count++;
        masSeq = realloc(masSeq, sizeof(sequen) * sub_count);
        masSeq[sub_count - 1] = str_to_seq(cmd);
    }
    // преобразуем весь массив последовтаельностей в одну структуру
    sequen_arr massiv; // структура из двух полей: массив(команд,рекур) и длина
    massiv.arr = masSeq;
    massiv.len = sub_count;
    free(cmd);
    return massiv;
}


char *get_str(){
    int lenth = 0;
    char *buffer = malloc(size);
    while (1)
    {
        for (int i = 0; i < size; i++)
        {
            *(buffer + lenth) = getchar();
            if ((*(buffer + lenth)) == '\n' || (*(buffer + lenth)) == EOF)
            {
                *(buffer + lenth) = '\0';

                return buffer;
            }
            lenth++;
        }
        buffer = realloc(buffer, lenth + size);
        if (buffer == NULL)
        {
            free(buffer);
            return NULL;
        }
    }

}

int main(){
    signal(SIGINT, SIG_IGN);
    while(1){
        printf("> ");
        char* str = get_str();
        if (str == NULL)
        {
            perror("Memory allocation error\n");
            exit(1);
        }
        printf("SSS %d\n", str[strlen(str) - 1]);
        if (str[strlen(str) - 1] == '&'){
            printf("RHJK\n");
            str[strlen(str) - 1] = '\0';
            pid_t p;
            pid_t pid = fork();
            if (pid == 0){
                p = fork();
                if (p == 0){
                    setpgid(0,0);
                    signal(SIGINT, SIG_DFL);
                    int tmp = open("/dev/null",O_RDONLY);
                    dup2(tmp,0);
                    close(tmp);
                    seqCommand(pars(str));
                }
                exit(0);
            }
            else{
                int st;
                waitpid(pid, &st, 0);
                continue;
            }
        }
        else{
        sequen_arr mas = pars(str); // получили структуру из массива аргументов(+нужна ли рекурсия) и длины
        seqCommand(mas);
        }
        free(str);
    }
}
