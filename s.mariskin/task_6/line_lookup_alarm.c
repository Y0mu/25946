/* Используем таблицу из задачи 5 и отводим пять секунд на каждый ввод */
int run_lookup_io(int argc, char **argv, unsigned int seconds);

int main(int argc, char **argv)
{
    return run_lookup_io(argc, argv, 5);
}
