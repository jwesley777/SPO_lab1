void normalize_path(char *normalized_path) {
    int path_normalized = 0;
    int i = 0;

    while (!path_normalized)
    {
        if (i + 1 < strlen(normalized_path) && ( (i == 0 && normalized_path[i] == '.' && normalized_path[i + 1] == '/')
            || (i != 0 && normalized_path[i - 1] == '/' && normalized_path[i] == '.' && normalized_path[i + 1] == '/') ))
        {
            if (i < strlen(normalized_path) - 2)
            {
                int u = i;
                for (; u < strlen(normalized_path) - 2; u++)
                {
                    normalized_path[u] = normalized_path[u + 2];
                }
                normalized_path[u] = 0;
                normalized_path[u + 1] = 0;
            }
            else
            {
                normalized_path[i] = 0;
                normalized_path[i + 1] = 0;
            }
        }
        else if ((i == 0 || normalized_path[i - 1] == '/') && i + 1 == strlen(normalized_path) && normalized_path[i] == '.')
        {
            if (i != 0)
            {
                normalized_path[i - 1] = 0;  
            }
            normalized_path[i] = 0;              
        }
        else
        {
            if (i >= strlen(normalized_path))
            {
                path_normalized = 1;
            }
            i++;
        }
    }
}

void remove_back_directories(char *normalized_path) {
    int path_normalized = 0;
    int i = 0;

    while (!path_normalized)
    {
        if (i + 2 < strlen(normalized_path) && normalized_path[i] == '/' && normalized_path[i + 1] == '.' 
            && normalized_path[i + 2] == '.' && (i + 3 == strlen(normalized_path) || normalized_path[i + 3] == '/')) 
        {
            int u = i - 1;
            while (normalized_path[u] != '/')
            {
                u--;
            }
            unsigned int offset = i - u + 3;
            i = u;
            for (; u < strlen(normalized_path) - offset; u++)
            {
                normalized_path[u] = normalized_path[u + offset];
            }
            unsigned int length = strlen(normalized_path);
            for (u = strlen(normalized_path) - offset; u < length; u++)
            {
                normalized_path[u] = 0;
            }
        }
        else
        {
            if (i >= strlen(normalized_path))
            {
                path_normalized = 1;
            }
            i++;
        }
    }

    if (normalized_path[0] == 0)
    {
        normalized_path[0] = '/';
    }
}