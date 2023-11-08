clear; clf;
size = 1000;
precision = 1e-3;
arr_a = zeros(size);

for i = 1:size
    for j = 1:size
        if (i==0 || j==0 || i==size || j==size)
            arr_a(i,j) = (i-1)*(j-1);
        end
    end
end

arr_b = arr_a;

arr_res = avg(arr_a, arr_b, precision, size);

disp("Hello, World!");