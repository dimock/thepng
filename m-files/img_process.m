% solve test task

function do_process(varargin)
    thrCoef = 1.1;
    in_name = 'problem_2.png';
    out_name = 'my_result.png';
    img = imread(in_name);
    img_gray = rgb2gray(img);
    img_dbl = im2double(img_gray);
    img_dbl_comp = imcomplement(img_dbl);
    % find threshold for background color
    Thr = graythresh(img_dbl)*thrCoef;
    bw_mask = im2bw(img_dbl, Thr);
    % HACK - add right part of ellipse to mask. it is incomplete yet
    bw_mask = im2bw(img_dbl_comp, 0.05) .* bw_mask;
    % background mask
    img_back = bw_mask .* img_dbl;
    % sum all non-zero points to calculate average background color
    S = 0;
    num = 0;
    W=size(img_back, 2);
    H=size(img_back, 1);
    y_ellipse_start = H;
    y_ellipse_stop = 0;
    xleft_ellipse = H;
    xright_ellipse = 0;
    segment_n = 0;
    % search for ellipse coords
    % also calculate sum of colors in this loop
   for y=1:H
        % find horizontal line, what ellipse starts from
        if y_ellipse_start == H
            p = y_line_black( y, img_back);
            if segment_n == 0 && p == 0
                segment_n = 1;
            elseif segment_n == 1 && p ~= 0
                y_ellipse_start = y;
                segment_n = 2;
            end
        elseif y_ellipse_stop == 0 && segment_n > 1
            p = y_line_black( y, img_back);
            if p == 0 && segment_n == 2
                segment_n = 3;
            end
            if p ~= 0 && segment_n == 3
                y_ellipse_stop = y;
            end
        end
        % find left and right ellipse boundaries
        for x=1:W
            v = img_back(y, x);
            if y > y_ellipse_start && v == 0
                if x < xleft_ellipse
                    xleft_ellipse = x;
                elseif x > xright_ellipse
                    xright_ellipse = x;
                end
            end
            if v == 0
                continue
            end
            S = S + v;
            num = num + 1;
        end
    end
    % search for rectanle left coord
    xleft_rect = 0;
    segment_n = 0;
    for x=1:W
        p = x_line_black(x, y_ellipse_start, bw_mask);
        if p == 0 && segment_n == 0
            segment_n = 1;
        elseif p ~= 0 && segment_n == 1
            xleft_rect = x;
            break;
        end
    end
    % collect pixels of rect mask
    rect_mask = ones(size(img_back));
    for y = 1:y_ellipse_start-1
        rect_mask(y, xleft_rect:W) = bw_mask(y,xleft_rect:W);
    end
    rect_mask = imcomplement(rect_mask);
    % make ellipse mask complete
    xcenter_ellipse = (xleft_ellipse + xright_ellipse) / 2;
    for y=y_ellipse_start:y_ellipse_stop
        for x=1:W
            v = bw_mask(y, x);
            if v == 0 && x < xcenter_ellipse
                x1 = 2*xcenter_ellipse - x;
                bw_mask(y, x1) = 0;
            end
        end
    end
    % other figures (ellipse and scribble)
    figures_mask = bw_mask + rect_mask;
    figures_mask = imcomplement(figures_mask);
    M = S / num;
    img_denoise_bg = (ones(size(img_back)) * M) .* bw_mask;
    img_figures = img_dbl .* figures_mask;
    img_rect = img_dbl .* rect_mask;
    img_result = img_denoise_bg + img_figures + img_rect;
    figure(5), imshow(img_result);
    figure(2), imshow(img_rect);
    figure(3), imshow(img_figures);
    imwrite(img_result, out_name, 'png');
end

% does line y have at least one black point
function p = y_line_black(y, mask)
    y_line_mask = mask(y, :);
    p = prod(y_line_mask);
end

function p = x_line_black(x, ymax, mask)
    x_line_mask = mask(1:ymax, x);
    p = prod(x_line_mask);
end
