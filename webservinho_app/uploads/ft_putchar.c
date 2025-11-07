#include <unistd.h>
#include <stdio.h>

void	ft_putchar(char c)
{
	write(1, &c, 1);
}

void	ft_print_alpha(void)
{
	char	c;

	c = 'a';
	while (c <= 'd')
	{
		write(1, &c, 1);
		c = c + 1;
		//c++;
	}
}

void	ft_div(int a, int b, int *div, int *mod)
{
	*div = a / b;
	*mod = a % b;
}

int	ft_putstr(char *str)
{
	int	i;

	i = 0;
	while (str[i] != '\0')
	{
		write(1, &str[i], 1);
		i++;
	}
	return (i);
}

int	main(void)
{
	//ft_putchar('b');
	//ft_print_alpha();
/* 	int div;
	int mod;
	ft_div(10, 3, &div, &mod);
	printf("div -> %d | mod -> %d\n", div, mod); */
	int len = ft_putstr("Banana!");
	printf("\ntamanho %d", len);
	return (0);
}

