function arr_res = avg(arr_src, arr_dst, precision, size)
    counter = 0;
    while (1)
        counter = counter + 1;
        precision_met = 1;
        for i = 2:(size-1)
            for j = 2:(size-1)
                arr_dst(i,j) = (arr_src(i,j-1) + arr_src(i,j+1) + arr_src(i-1,j) + arr_src(i+1,j))/4;
                precision_met = precision_met & (abs(arr_src(i,j) - arr_dst(i,j)) <= precision);
            end
        end
        
        tmp_arr = arr_src;
        arr_src = arr_dst;
        arr_dst = tmp_arr;
        arr_res = arr_src;

        if precision_met==1 
            break;
        end
    end
    fprintf("%d iterations\n",counter);
end