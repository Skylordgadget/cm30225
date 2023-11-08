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

counter = 0;

while 1
    counter = counter + 1;
    [arr_N, arr_E, arr_S, arr_W] = deal(arr_a);
    
    arr_N = padarray(arr_a, [2,1], 0, 'post');
    arr_N = padarray(arr_N, [0,1], 0, 'pre');
    
    arr_E = padarray(arr_a, [1,2], 0, 'pre');
    arr_E = padarray(arr_E, 1, 0, 'post');
    
    arr_S = padarray(arr_a, [2,1], 0, 'pre');
    arr_S = padarray(arr_S, [0,1], 0, 'post');
    
    arr_W = padarray(arr_a, [1,2], 0, 'post');
    arr_W = padarray(arr_W, 1, 0, 'pre');
    
    arr_res = arr_N + arr_E + arr_S + arr_W;
    arr_res = arr_res(3:end-2, 3:end-2);
    arr_res = arr_res ./ 4;
    
    precision_met = abs(arr_a(2:end-1, 2:end-1) - arr_res) <= precision;
    if precision_met == 1
        break
    end 
    
    arr_a(2:end-1, 2:end-1) = arr_res;
end
arr_a(2:end-1, 2:end-1) = arr_res;

