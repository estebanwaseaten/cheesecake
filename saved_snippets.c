/*	struct device_node *root_node = NULL;
	root_node = of_find_all_nodes(NULL);
	if( root_node != NULL )
	{
		pr_info("Found root node!!! \n");

		//char *propValue;
		int size;
		of_property_read_string()
		const char *propValue = of_get_property(root_node, "compatible", &size);
		if ( propValue == NULL)
		{
			pr_info("could not find property\n");
			return -1;
		}
		/*else
		{
			for (size_t i = 0; i < size; i++)
			{
				printk( "%c", *(char*)(propValue + i));
			}
		}
	}*/
