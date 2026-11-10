/* Общий код чтения файла находится в common/lookup_io.c */
int run_lookup_io(int argc, char **argv, unsigned int seconds);

int main(int argc, char **argv)
{
    return run_lookup_io(argc, argv, 0);
}
