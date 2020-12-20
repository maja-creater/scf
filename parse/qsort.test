int printf(char* fmt, int a);

int sort(int* a, int m, int n)
{
	if (m >= n)
		return 0;

	int i = m;
	int j = n;
	int t;

	t = a[i];
	while (i < j) {

		while (i < j && t < a[j])
			j--;
		a[i]  = a[j];
		a[j]  = t;

		while (i < j && a[i] < t)
			i++;
		a[j]  = a[i];
		a[i]  = t;
	}

	sort(a, m, i - 1);
	sort(a, i + 1, n);
	return 0;
}

int main()
{
	int a[10] = {1, 3, 5, 7, 9, 8, 6, 4, 2, 0};

	sort(a, 0, 9);

	int i;
	for (i = 0; i < 10; i++)
		printf("%d\n", a[i]);

	return 0;
}
