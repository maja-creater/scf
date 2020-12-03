int printf(char* fmt, int a);

class Point
{
	int x;
	int y;
};

int cmp_pt(Point* v0, Point* v1);

int cmp_point(Point* p0, Point* p1)
{
	if (p0->x < p1->x)
		return -1;
	else if (p0->x > p1->x)
		return 1;
	return 0;
}

int sort(Point** a, cmp_pt* cmp, int m, int n)
{
	if (m >= n)
		return 0;

	int i = m;
	int j = n;

	Point* t = a[i];

	while (i < j) {

		while (i < j && cmp(t, a[j]) < 0)
			j--;

		a[i] = a[j];
		a[j] = t;

		while (i < j && cmp(a[i], t) < 0)
			i++;
		a[j] = a[i];
		a[i] = t;
	}

	sort(a, cmp, m,     i - 1);
	sort(a, cmp, i + 1, n);
	return 0;
}

int main()
{
	Point p0 = {0, 1};
	Point p1 = {4, 1};
	Point p2 = {2, 1};
	Point p3 = {3, 1};
	Point p4 = {1, 1};

	Point* pp[5] = {&p0, &p1, &p2, &p3, &p4};

	Point** a = (Point**)pp;

	sort(a, cmp_point, 0, 4);

	int i;
	for (i = 0; i < 5; i++) {
		printf("%d\n", pp[i]->x);
	}

	return 0;
}
