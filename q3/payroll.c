#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define N 100

typedef struct {
    int id;
    double hours;
    double rate;
} Employee;

int main(void) {
    int p1[2], p2[2], r1[2], r2[2];
    /* p1: parent -> child1 (employee data)   r1: child1 -> parent (basic salary)
       p2: parent -> child2 (employee data)   r2: child2 -> parent (deductions)  */
    if (pipe(p1) == -1 || pipe(p2) == -1 || pipe(r1) == -1 || pipe(r2) == -1) {
        perror("pipe");
        exit(1);
    }

    pid_t c1 = fork();
    if (c1 < 0) { perror("fork"); exit(1); }
    if (c1 == 0) {                         /* Child 1: basic salary */
        close(p1[1]); close(r1[0]);
        close(p2[0]); close(p2[1]);
        close(r2[0]); close(r2[1]);
        Employee e;
        double basic;
        while (read(p1[0], &e, sizeof(e)) == sizeof(e)) {
            basic = e.hours * e.rate;
            write(r1[1], &basic, sizeof(basic));
        }
        close(p1[0]); close(r1[1]);
        exit(0);
    }

    pid_t c2 = fork();
    if (c2 < 0) { perror("fork"); exit(1); }
    if (c2 == 0) {                         /* Child 2: deductions (15%) */
        close(p2[1]); close(r2[0]);
        close(p1[0]); close(p1[1]);
        close(r1[0]); close(r1[1]);
        Employee e;
        double ded;
        while (read(p2[0], &e, sizeof(e)) == sizeof(e)) {
            ded = 0.15 * e.hours * e.rate;
            write(r2[1], &ded, sizeof(ded));
        }
        close(p2[0]); close(r2[1]);
        exit(0);
    }

    /* Parent */
    close(p1[0]); close(p2[0]);
    close(r1[1]); close(r2[1]);

    Employee emp[N];
    for (int i = 0; i < N; i++) {
        emp[i].id = i + 1;
        emp[i].hours = 160;
        emp[i].rate = 10 + (i % 5);
        write(p1[1], &emp[i], sizeof(Employee));
        write(p2[1], &emp[i], sizeof(Employee));
    }
    close(p1[1]); close(p2[1]);

    double basic[N], ded[N];
    for (int i = 0; i < N; i++) {
        read(r1[0], &basic[i], sizeof(double));
        read(r2[0], &ded[i], sizeof(double));
    }
    close(r1[0]); close(r2[0]);

    wait(NULL); wait(NULL);

    printf("ID\tBasic\t\tDeductions\tFinal\n");
    for (int i = 0; i < N; i++)
        printf("%d\t%.2f\t\t%.2f\t\t%.2f\n", emp[i].id, basic[i], ded[i], basic[i] - ded[i]);
    return 0;
}
