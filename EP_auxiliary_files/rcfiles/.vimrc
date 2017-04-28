set ai ts=4 sw=4 wm=5 sm
"set ai
"set tabstop=2
"set shiftwidth=2
"set expandtab
"set smarttab
"highlight OverLength ctermbg=red ctermfg=white guibg=#592929
"match OverLength /\%>80v.\+/
set nu
set hlsearch
set ic
set textwidth=80
set colorcolumn=+1
highlight ColorColumn guibg=#2d2d2d ctermbg=246
inoremap <C-s> <esc>:w<cr>a
nnoremap <C-s> :w<cr>
"autocmd FileType c,cpp autocmd BufWritePre <buffer> :%s/\s\+$// "to remove trailing spaces
"ctrl-t to remove trailing white spaces
inoremap <C-l> <esc>:%s/\s\+$//<cr>a
nnoremap <C-l> :%s/\s\+$//<cr>
set incsearch "incremental search
syntax enable

"Comment these two later"
"set background=dark
"colorscheme solarized

set t_Co=256
set backspace=2
set ruler
