#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

char *all_sep[] = {" ", "\n", ">", ">>", "<", "&", "|"};

char *instr[] = {">", ">>", "<", "&", "|"};

enum
{
    len_all_sep = 7
};

enum
{
    chunk_size = 8
};

enum status_str
{
    st_cont_str,
    st_exit_str
};

enum type_error
{
    err_quotes,
    err_execute,
    err_sep,
    err_amper,
    err_no
};

struct list_of_str
{
    char *str;
    char sep;
    struct list_of_str *next;
};

struct chunk_of_str
{
    char array[chunk_size];
    int len;
    struct chunk_of_str *next;
};

struct command_line
{
    char **prog_name;
    char *right_operation;
    char *left_operation;
    char *file_to_read;
    char *file_to_write;
    struct command_line *next;
};

void print_error(enum type_error error)
{
   // There should be a switch (error)
    if (error != err_no)
        fprintf(stderr,"Bad Input \n");
}

struct chunk_of_str* init_chunk()
{
    int i = 0;
    struct chunk_of_str *first_chunk;
    first_chunk = malloc(sizeof(struct chunk_of_str));
    first_chunk->next = NULL;
    first_chunk->len = 0;
    for(i = 0; i < chunk_size; i++)
        first_chunk->array[i] = 0;
    return first_chunk;
}

struct list_of_str* init_list_of_str()
{
     struct list_of_str *lstr;
     lstr = malloc(sizeof(struct list_of_str));
     lstr->sep = 0;
     lstr->next = NULL;
     lstr->str = NULL;
     return lstr;
}

struct chunk_of_str* put_char_in(struct chunk_of_str *chunk, int c)
{
    if (chunk)
    {
        if (chunk->len == chunk_size)
        {
            chunk->next = init_chunk();
            chunk = chunk -> next;
        }
        chunk->array[chunk -> len] = c;
        (chunk->len)++;
    }

    return chunk;
}

int lenArgV(char **argV)
{
    int i = 0, count = 0;
    while(argV[i] != NULL)
    {
        count++;
        i++;
    }
    return count;
}

int get_char_from(struct chunk_of_str **cur, int index)
{
    int save_for_return;
    if ((*cur) && ((index) % chunk_size < ((*cur)->len)))
    {
        save_for_return = ((*cur)->array)[(index) % chunk_size];
        if ((index) % chunk_size == chunk_size - 1)
            (*cur) = (*cur)->next;
        return save_for_return;
    }
    return -1;
}
char *str_copy(const char *str)
{
    int i = 0;
    char *str2;
    if (str == NULL)
        return NULL;
    while(str[i])
        i++;
    str2 = malloc(i + 1);
    i = 0;
    while(str[i])
    {
        str2[i] = str[i];
        i++;
    }
    str2[i] = 0;
    return str2;
}

char str_equal(const char *str1, const char *str2)
{
    enum {not_equal = 0, equal = 1};
    int i = -1;
    if ((str1 && !str2) || (!str1 && str2))
        return not_equal;
    do
    {
        i++;
        if (str1[i] != str2[i])
            return not_equal;
    }while((str1[i] != 0) && (str2[i] != 0));
    return equal;
}

int len_chunk(struct chunk_of_str *first_chunk)
{
    int res = 0;
    while(first_chunk)
    {
        res = res + (first_chunk->len);
        first_chunk = first_chunk->next;
    }
    return res;
}

char find_char_in(char *str, int search)
{
    int i = 0;
    while(str[i])
    {
        if (str[i] == search)
            return 1;
        i++;
    }
    return 0;
}

int len_sep(char *str, char **all_sep)
{
    int max_len = 0, i = 0, j = 0;
    for(j = 0; j < len_all_sep; j++)
    {
        i = 0;
        while(str[i] && all_sep[j])
        {
            if (str[i] - all_sep[j][i])
                break;
            i++;

        }
        if (max_len < i)
            max_len = i;
    }
    return max_len;
}
char any_str_equal(char *str, char **argV)
{
    int i = 0;
    while(argV[i])
    {
        if (str_equal(argV[i],str))
            return 1;
        i++;
    }
    return 0;
}

int get_index_of_end_word(char *str)
{
    int i = 0;
    char quotes = 0;
    if (str[i] == 0)
        return 0;
    if (len_sep(str, all_sep))
        return len_sep(str, all_sep) - 1;
    while(quotes || (!quotes && !len_sep(str + i, all_sep) && str[i]))
    {
        if (str[i] == '"')
            quotes = (quotes + 1) % 2;
        i++;
    }
    return (i-1);
}

char* del_char(char *str, char* for_del)
{
    int i = 0, j = 0, len = 0;
    char *res = NULL;
    while(str[i])
    {
        if(!find_char_in(for_del, str[i]))
            len++;
        i++;
    }
    if (i == len)
        return str;
    i = 0;
    res = malloc(len + 1);
    while(str[i])
    {
        if(!find_char_in(for_del, str[i]))
        {
            res[j] = str[i];
            j++;
        }
        i++;
    }
    res[j] = 0;
    free(str);
    return res;
}

void free_chunk( struct chunk_of_str *first_chunk)
{
    if (first_chunk)
    {
        free_chunk(first_chunk -> next);
        free(first_chunk);
    }
}

char* transform_chunk2str(struct chunk_of_str *first_chunk)
{
    char *str;
    int c = 0, i = 0, len = 0;
    len = len_chunk(first_chunk);
    str = malloc(len + 1);
    while((c = get_char_from(&first_chunk, i)) != -1)
    {
        str[i] = c;
        i++;
    }
    str[i] = 0;
    return str;
}

enum status_str read_string(char **str)
{
    struct chunk_of_str *tmp = NULL, *first_chunk = NULL;
    char *res = NULL, c;
    enum status_str status = st_cont_str;
    first_chunk = init_chunk();
    tmp = first_chunk;
    do
    {
        c = getchar();
        if (c == EOF)
        {
            status = st_exit_str;
            break;
        }
        tmp = put_char_in(tmp, c);
    }while(c != '\n');
    res = transform_chunk2str(first_chunk);
    free_chunk(first_chunk);
    *str = res;
    return status;
}

void free_lstr(struct list_of_str *lstr)
{
    if (lstr)
    {
        free_lstr(lstr->next);
        free(lstr->str);
        free(lstr);
    }
}

int count_quotes(char *str)
{
    int i = 0, count = 0;;
    while(str[i])
    {
        if (str[i] == '"')
            count++;
        i++;
    }
    return count;
}

struct list_of_str* del_last(struct list_of_str *lstr)
{
    if (lstr == NULL)
        return NULL;
    else if (lstr->next == NULL)
    {
        free_lstr(lstr);
        return NULL;
    }
    else if ((lstr->next)->next == NULL)
    {
        lstr->next = del_last(lstr->next);
        return lstr;
    }
    del_last(lstr->next);
    return lstr;
}

char* get_word(char *str, int end_index)
{
    char *res = NULL;
    int i = 0, len = end_index + 1;
    res = malloc(len + 1);
    while(i < len)
    {
        res[i] = str[i];
        i++;
    }
    res[i] = 0;
    return res;
}

void print_lstr(struct list_of_str *lstr)
{
    if (lstr)
    {
        printf("%s",lstr -> str);
/*      putchar('.'); */
        if (lstr->next)
        {
            printf("\n");
        }
        print_lstr(lstr->next);
    }
}

struct list_of_str* transform_str_2_list_of_word(char* str)
{
    struct list_of_str *tmp = NULL, *first_lstr = NULL;
    int  start_index = 0, end_index = 0;
    char  *word = NULL, *empty = " ";
    first_lstr = init_list_of_str();
    tmp = first_lstr;
    do
    {
        end_index = get_index_of_end_word(str + start_index);
        word = get_word(str + start_index, end_index);
        if (any_str_equal(word, instr))
            tmp->sep = 1;
        word = del_char(word, "\"");
        if (word[0] && !str_equal(word," ") && !str_equal(word,"\n"))
        {
            tmp->str = word;
            tmp->next = init_list_of_str();
            tmp = tmp->next;
        }
        else if (word[0])
        {
            free(word);
            word = empty;
        }
        start_index = start_index + end_index + 1;
    }while(word[0]);
    free(word);
    first_lstr = del_last(first_lstr);
    return first_lstr;
}

void execute_cd(char **argV)
{
    int res, len = lenArgV(argV);
    if (len == 1)
    {
        print_error(err_execute);
        return;
    }
    else if (len == 2)
    {
        res = chdir(argV[1]);
        if (res == -1)
            perror(argV[1]);
        return;
    }
}

void change_stream(char *operation, char *file_name)
{
    int fd, mode, stream;
    if (str_equal(operation,">"))
    {
        mode = O_WRONLY|O_CREAT|O_TRUNC;
        stream = 1;
    }
    else if (str_equal(operation,">>"))
    {
        mode = O_WRONLY|O_CREAT|O_APPEND;
        stream = 1;
    }
    else if(str_equal(operation,"<"))
    {
        mode = O_RDONLY;
        stream = 0;
    }
    else
        return;
    fd = open(file_name, mode, 0666);
    if (fd == -1)
    {
        perror(file_name);
        exit(127);
    }
    dup2(fd, stream);
    close(fd);
}

int count_vert(struct command_line *cmd)
{
    int count = 0;
    while(cmd)
    {
        if (str_equal(cmd->right_operation, "|"))
            count++;
        cmd = cmd->next;
    }
    return count;
}

char find_in_array(int *array_pid, int len, int find)
{
    int i = 0;
    for(i = 0; i < len; i++)
        if (find == array_pid[i])
            return i + 1;
    return 0;
}

void wait_pipeline(int *array_pid, int len)
{
    int count = 0, pid;
    while(count != len)
    {
        pid = wait(NULL);
        if (find_in_array(array_pid, len, pid))
            count++;
    }
    free(array_pid);
}

void execvp_either_exit(char **prog_name)
{
    execvp(prog_name[0], prog_name);
    perror(prog_name[0]);
    exit(1);
}

void equate_arrays(int *arr1, int *arr2, int len)
{ /* suppose that len array equal */
    int i = 0;
    for(i = 0; i < len; i++)
        arr1[i] = arr2[i];
}

void fork_either_try(int *array_pid, int index)
{
    array_pid[index] = fork();
    while (array_pid[index] == -1)
    {
        perror("fork");
        sleep(1);
        array_pid[index] = fork();
    }
}

void pipeline(struct command_line *cmd, int *array_pid, int len)
{
    int fd[2], i = 1;
    pipe(fd);
    fork_either_try(array_pid, 0);
    if (array_pid[0] == 0)
    {
        change_stream(cmd->left_operation, cmd->file_to_read);
        close(fd[0]);
        dup2(fd[1], 1);
        close(fd[1]);
        execvp_either_exit(cmd->prog_name);
    }
    cmd = cmd->next;
    close(fd[1]);
    for (i = 1; i < len; i++)
    {
        int fd_next[2];
        pipe(fd_next);
        fork_either_try(array_pid, i);
        if (array_pid[i] == 0)
        {
            dup2(fd[0], 0);
            close(fd[0]);
            dup2(fd_next[1],1);
            close(fd_next[1]);
            close(fd_next[0]);
            execvp_either_exit(cmd->prog_name);
        }
        close(fd[0]);
        close(fd_next[1]);
        equate_arrays(fd, fd_next, 2);
        cmd = cmd->next;
    }
    fork_either_try(array_pid, len);
    if (array_pid[len] == 0)
    {
        change_stream(cmd->right_operation,cmd->file_to_write);
        dup2(fd[0], 0);
        close(fd[0]);
        execvp_either_exit(cmd->prog_name);
    }
    close(fd[0]);
}

void execute(struct command_line *cmd, char background)
{
    int  *array_pid = NULL, len = count_vert(cmd);
    if (cmd->prog_name && str_equal(cmd->prog_name[0], "cd"))
    {
        execute_cd(cmd->prog_name);
        return;
    }
    array_pid = malloc((len + 1)*sizeof(int));
    if (str_equal(cmd->right_operation,"|"))
        pipeline(cmd, array_pid, len);
    else
    {
        fork_either_try(array_pid, 0);
        if (array_pid[0] == 0)   /*child*/
        {
            change_stream(cmd->left_operation, cmd->file_to_read);
            change_stream(cmd->right_operation, cmd->file_to_write);
            execvp_either_exit(cmd->prog_name);
        }
    }
    if (!background)
        wait_pipeline(array_pid, len + 1);
    else
        free(array_pid);
}

char** transform_lstr_2_argv(struct list_of_str *lstr, int len)
{
    char **res = NULL;
    int i = 0;
    res = malloc((len + 1)*sizeof(char*));
    while(lstr && i < len)
    {
        res[i] = lstr->str;
        i++;
        lstr = lstr->next;
    }
    res[i] = NULL;
    return res;
}

struct command_line* init_command_line()
{
    struct command_line *cmd;
    cmd = malloc(sizeof(struct command_line));
    cmd->prog_name = NULL;
    cmd->right_operation = NULL;
    cmd->left_operation = NULL;
    cmd->file_to_read = NULL;
    cmd->file_to_write = NULL;
    cmd->next = NULL;
    return cmd;
}

void set_left_arrw(struct list_of_str *lstr, struct command_line *cmd)
{
    while(lstr)
    {
        if (lstr->sep && str_equal(lstr->str,"<"))
        {
            free(cmd->left_operation);
            cmd->left_operation = str_copy(lstr->str);
            cmd->file_to_read = str_copy((lstr->next)->str);
            break;
        }
        lstr = lstr->next;
    }
}

void except_left_arrw(struct list_of_str *lstr)
{
    struct list_of_str *tmp = NULL;
    while(lstr && lstr->next)
    {
        if ((lstr->next)->sep && str_equal((lstr->next)->str,"<"))
        {
            tmp = lstr->next;
            lstr->next = (tmp->next)->next;
            free((tmp->next)->str);
            free(tmp->next);
            free(tmp->str);
            free(tmp);
        }
        lstr = lstr->next;
    }
}

void set_operation(struct list_of_str *lstr, struct command_line *cmd)
{
    char *str1 = NULL;
    set_left_arrw(lstr, cmd);
    except_left_arrw(lstr);
    while(lstr)
    {
        str1 = lstr->str;
        if((str_equal(str1,">") || str_equal(str1,">>")) && lstr->sep)
        {
            cmd->right_operation = str_copy(str1);
            lstr = lstr->next;
            cmd->file_to_write = str_copy(lstr->str);
            break;
        }
        if (str_equal(lstr->str,"|") && lstr->sep)
        {
            cmd->right_operation = str_copy(str1);
            cmd->next = init_command_line();
            cmd = cmd->next;
            cmd->left_operation = str_copy(str1);
        }
        lstr = lstr->next;
    }
}

struct command_line* transform_lstr_2_cmd(struct list_of_str *lstr)
{
    struct command_line *cmd = NULL, *first_cmd;
    struct list_of_str  *tmp;
    int len = 0;
    char *str1 = NULL;
    cmd = init_command_line();
    first_cmd = cmd;
    set_operation(lstr, cmd); /* set left_operation, right_operation
                                fite_to_read, file_to_write */
    tmp = lstr;
    while(lstr)
    {
        len++;
        if ((lstr->next && ((lstr->next)->sep)) || !lstr->next)
        {
            cmd->prog_name = transform_lstr_2_argv(tmp, len);
            if (lstr->next)
                tmp = (lstr->next)->next;
            cmd = cmd->next;
            lstr = lstr->next;
            if (lstr == NULL)
                break;
            len = 0;
        }
        str1 = lstr->str;
        if ((str_equal(str1,">") || str_equal(str1,">>")) && lstr->sep)
            break;
        lstr = lstr->next;
    }
    return first_cmd;
}

int count_sep(struct list_of_str *lstr, char *sep)
{
    int count = 0;
    while(lstr)
    {
        if (str_equal(lstr->str, sep) && lstr->sep)
            count++;
        lstr = lstr->next;
    }
    return count;
}

char is_valid_list_of_str(struct list_of_str *lstr)
{
    int right = count_sep(lstr,">");
    int left = count_sep(lstr,"<");
    int two_right = count_sep(lstr,">>");
    int amper = count_sep(lstr,"&");

    if (lstr == NULL)
        return 0;
    if (right && two_right)
    {
        print_error(err_sep);
        return 0;
    }
    if (right > 1 || two_right > 1 || left > 1 || amper > 1)
    {
        print_error(err_sep);
        return 0;
    }
    if (lstr->sep)
    {
        print_error(err_sep);
        return 0;
    }

    while(lstr)
    {
        if (lstr->sep && str_equal("&",lstr->str) && lstr->next)
        {
            print_error(err_amper);
            return 0;
        }
        if (lstr->sep && !str_equal("&",lstr->str) && (
             !lstr->next || lstr->next->sep))
        {
            print_error(err_sep);
            return 0;
        }
        lstr = lstr->next;
    }
    return 1;
}

void free_cmd(struct command_line *cmd)
{
    if (cmd)
    {
        free_cmd(cmd->next);
        if (cmd->prog_name)
            free(cmd->prog_name);
        if (cmd->left_operation)
            free(cmd->left_operation);
        if (cmd->right_operation)
            free(cmd->right_operation);
        if (cmd->file_to_write)
            free(cmd->file_to_write);
        if (cmd->file_to_read)
            free(cmd->file_to_read);
        free(cmd);
    }
}

int main()
{
    struct list_of_str *lstr = NULL;
    enum status_str status;
    char *str = NULL;
    struct command_line *cmd = NULL;
    do
    {
        lstr = NULL;
        printf("Input:");
        status = read_string(&str);
        if (count_quotes(str) % 2)
        {
            free(str);
            print_error(err_quotes);
            continue;
        }
        lstr = transform_str_2_list_of_word(str);
        if (is_valid_list_of_str(lstr))
        {
            char background;
            background = count_sep(lstr,"&");
            if (background)
                lstr = del_last(lstr);
            cmd = transform_lstr_2_cmd(lstr);
            execute(cmd, background);
            free_cmd(cmd);
        }
        free(str);
        free_lstr(lstr);
        while(wait4(-1, NULL, WNOHANG, NULL) > 0)
            {}
    }while(status != st_exit_str);
    return 0;
}

